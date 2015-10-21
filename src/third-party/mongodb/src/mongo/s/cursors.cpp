// cursors.cpp
/*
 *    Copyright (C) 2010 10gen Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kQuery

#include "mongo/platform/basic.h"

#include "mongo/s/cursors.h"

#include <boost/scoped_ptr.hpp>
#include <string>
#include <vector>

#include "mongo/base/data_cursor.h"
#include "mongo/db/audit.h"
#include "mongo/db/auth/action_set.h"
#include "mongo/db/auth/action_type.h"
#include "mongo/db/auth/authorization_manager.h"
#include "mongo/db/auth/authorization_session.h"
#include "mongo/db/auth/privilege.h"
#include "mongo/client/connpool.h"
#include "mongo/db/commands.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/max_time.h"
#include "mongo/db/server_parameters.h"
#include "mongo/util/concurrency/task.h"
#include "mongo/util/log.h"
#include "mongo/util/net/listen.h"

namespace mongo {

using boost::scoped_ptr;
using std::endl;
using std::string;
using std::stringstream;

const int ShardedClientCursor::INIT_REPLY_BUFFER_SIZE = 32768;

// Note: There is no counter for shardedEver from cursorInfo since it is deprecated
static Counter64 cursorStatsMultiTarget;
static Counter64 cursorStatsSingleTarget;

// Simple class to report the sum total open cursors = sharded + refs
class CursorStatsSum {
public:
    operator long long() const {
        return get();
    }
    long long get() const {
        return cursorStatsMultiTarget.get() + cursorStatsSingleTarget.get();
    }
};

static CursorStatsSum cursorStatsTotalOpen;

static ServerStatusMetricField<Counter64> dCursorStatsMultiTarget("cursor.open.multiTarget",
                                                                  &cursorStatsMultiTarget);
static ServerStatusMetricField<Counter64> dCursorStatsSingleTarget("cursor.open.singleTarget",
                                                                   &cursorStatsSingleTarget);
static ServerStatusMetricField<CursorStatsSum> dCursorStatsTotalOpen("cursor.open.total",
                                                                     &cursorStatsTotalOpen);


// --------  ShardedCursor -----------

ShardedClientCursor::ShardedClientCursor(QueryMessage& q, ParallelSortClusteredCursor* cursor) {
    verify(cursor);
    _cursor = cursor;

    _skip = q.ntoskip;
    _ntoreturn = q.ntoreturn;

    _totalSent = 0;
    _done = false;

    _id = 0;

    if (q.queryOptions & QueryOption_NoCursorTimeout) {
        _lastAccessMillis = 0;
    } else
        _lastAccessMillis = Listener::getElapsedTimeMillis();

    cursorStatsMultiTarget.increment();
}

ShardedClientCursor::~ShardedClientCursor() {
    verify(_cursor);
    delete _cursor;
    _cursor = 0;
    cursorStatsMultiTarget.decrement();
}

long long ShardedClientCursor::getId() {
    if (_id <= 0) {
        _id = cursorCache.genId();
        verify(_id >= 0);
    }
    return _id;
}

int ShardedClientCursor::getTotalSent() const {
    return _totalSent;
}

void ShardedClientCursor::accessed() {
    if (_lastAccessMillis > 0)
        _lastAccessMillis = Listener::getElapsedTimeMillis();
}

long long ShardedClientCursor::idleTime(long long now) {
    if (_lastAccessMillis == 0)
        return 0;
    return now - _lastAccessMillis;
}

bool ShardedClientCursor::sendNextBatchAndReply(Request& r) {
    BufBuilder buffer(INIT_REPLY_BUFFER_SIZE);
    int docCount = 0;
    bool hasMore = sendNextBatch(r, _ntoreturn, buffer, docCount);
    replyToQuery(
        0, r.p(), r.m(), buffer.buf(), buffer.len(), docCount, _totalSent, hasMore ? getId() : 0);

    return hasMore;
}

bool ShardedClientCursor::sendNextBatch(Request& r,
                                        int batchSize,
                                        BufBuilder& buffer,
                                        int& docCount) {
    uassert(10191, "cursor already done", !_done);

    int maxSize = 1024 * 1024;
    if (_totalSent > 0)
        maxSize *= 3;

    docCount = 0;

    // If batchSize is negative, it means that we should send up to -batchSize results
    // back to the client, and that we should only send a *single batch*. An batchSize of
    // 1 is also a special case which means "return up to 1 result in a single batch" (so
    // that +1 actually has the same meaning of -1). For all other values of batchSize, we
    // may have to return multiple batches.
    const bool sendMoreBatches = batchSize == 0 || batchSize > 1;
    batchSize = abs(batchSize);

    // Set the initial batch size to 101, just like mongoD.
    if (batchSize == 0 && _totalSent == 0)
        batchSize = 101;

    // Set batch size to batchSize requested by the current operation unconditionally.  This is
    // necessary because if the loop exited due to docCount == batchSize then setBatchSize(0) was
    // called, so the next _cusor->more() will be called with a batch size of 0 if the cursor
    // buffer was drained the previous run.  Unconditionally setting the batch size ensures that
    // we don't ask for a batch size of zero as a side effect.
    _cursor->setBatchSize(batchSize);

    bool cursorHasMore = true;
    while ((cursorHasMore = _cursor->more())) {
        BSONObj o = _cursor->next();

        buffer.appendBuf((void*)o.objdata(), o.objsize());
        ++docCount;

        // Ensure that the next batch will never wind up requesting more docs from the shard
        // than are remaining to satisfy the initial batchSize.
        if (batchSize != 0) {
            if (docCount == batchSize)
                break;
            _cursor->setBatchSize(batchSize - docCount);
        }

        if (buffer.len() > maxSize) {
            break;
        }
    }

    // We need to request another batch if the following two conditions hold:
    //
    //  1. batchSize is positive and not equal to 1 (see the comment above). This condition
    //  is stored in 'sendMoreBatches'.
    //
    //  2. The last call to _cursor->more() was true (i.e. we never explicitly got a false
    //  value from _cursor->more()). This condition is stored in 'cursorHasMore'. If the server
    //  hits EOF while executing a query or a getmore, it will pass a cursorId of 0 in the
    //  query response to indicate that there are no more results. In this case, _cursor->more()
    //  will be explicitly false, and we know for sure that we do not have to send more batches.
    //
    //  On the other hand, if _cursor->more() is true there may or may not be more results.
    //  Suppose that the mongod generates enough results to fill this batch. In this case it
    //  does not know whether not there are more, because doing so would require requesting an
    //  extra result and seeing whether we get EOF. The mongod sends a valid cursorId to
    //  indicate that there may be more. We do the same here: we indicate that there may be
    //  more results to retrieve by setting 'hasMoreBatches' to true.
    bool hasMoreBatches = sendMoreBatches && cursorHasMore;

    LOG(5) << "\t hasMoreBatches: " << hasMoreBatches << " sendMoreBatches: " << sendMoreBatches
           << " cursorHasMore: " << cursorHasMore << " batchSize: " << batchSize
           << " num: " << docCount << " id:" << getId() << " totalSent: " << _totalSent << endl;

    _totalSent += docCount;
    _done = !hasMoreBatches;

    return hasMoreBatches;
}

// ---- CursorCache -----

long long CursorCache::TIMEOUT = 10 * 60 * 1000 /* 10 minutes */;
ExportedServerParameter<long long> cursorCacheTimeoutConfig(
    ServerParameterSet::getGlobal(), "cursorTimeoutMillis", &CursorCache::TIMEOUT, true, true);

unsigned getCCRandomSeed() {
    scoped_ptr<SecureRandom> sr(SecureRandom::create());
    return sr->nextInt64();
}

CursorCache::CursorCache() : _mutex("CursorCache"), _random(getCCRandomSeed()), _shardedTotal(0) {}

CursorCache::~CursorCache() {
    // TODO: delete old cursors?
    bool print = shouldLog(logger::LogSeverity::Debug(1));
    if (_cursors.size() || _refs.size())
        print = true;
    verify(_refs.size() == _refsNS.size());

    if (print)
        log() << " CursorCache at shutdown - "
              << " sharded: " << _cursors.size() << " passthrough: " << _refs.size() << endl;
}

ShardedClientCursorPtr CursorCache::get(long long id) const {
    LOG(_myLogLevel) << "CursorCache::get id: " << id << endl;
    scoped_lock lk(_mutex);
    MapSharded::const_iterator i = _cursors.find(id);
    if (i == _cursors.end()) {
        return ShardedClientCursorPtr();
    }
    i->second->accessed();
    return i->second;
}

int CursorCache::getMaxTimeMS(long long id) const {
    verify(id);
    scoped_lock lk(_mutex);
    MapShardedInt::const_iterator i = _cursorsMaxTimeMS.find(id);
    return (i != _cursorsMaxTimeMS.end()) ? i->second : 0;
}

void CursorCache::store(ShardedClientCursorPtr cursor, int maxTimeMS) {
    LOG(_myLogLevel) << "CursorCache::store cursor "
                     << " id: " << cursor->getId()
                     << (maxTimeMS != kMaxTimeCursorNoTimeLimit
                             ? str::stream() << "maxTimeMS: " << maxTimeMS
                             : string("")) << endl;
    verify(cursor->getId());
    verify(maxTimeMS == kMaxTimeCursorTimeLimitExpired || maxTimeMS == kMaxTimeCursorNoTimeLimit ||
           maxTimeMS > 0);
    scoped_lock lk(_mutex);
    _cursorsMaxTimeMS[cursor->getId()] = maxTimeMS;
    _cursors[cursor->getId()] = cursor;
    _shardedTotal++;
}

void CursorCache::updateMaxTimeMS(long long id, int maxTimeMS) {
    verify(id);
    verify(maxTimeMS == kMaxTimeCursorTimeLimitExpired || maxTimeMS == kMaxTimeCursorNoTimeLimit ||
           maxTimeMS > 0);
    scoped_lock lk(_mutex);
    _cursorsMaxTimeMS[id] = maxTimeMS;
}

void CursorCache::remove(long long id) {
    verify(id);
    scoped_lock lk(_mutex);
    _cursorsMaxTimeMS.erase(id);
    _cursors.erase(id);
}

void CursorCache::removeRef(long long id) {
    verify(id);
    scoped_lock lk(_mutex);
    _refs.erase(id);
    _refsNS.erase(id);
    cursorStatsSingleTarget.decrement();
}

void CursorCache::storeRef(const std::string& server, long long id, const std::string& ns) {
    LOG(_myLogLevel) << "CursorCache::storeRef server: " << server << " id: " << id << endl;
    verify(id);
    scoped_lock lk(_mutex);
    _refs[id] = server;
    _refsNS[id] = ns;
    cursorStatsSingleTarget.increment();
}

string CursorCache::getRef(long long id) const {
    verify(id);
    scoped_lock lk(_mutex);
    MapNormal::const_iterator i = _refs.find(id);

    LOG(_myLogLevel) << "CursorCache::getRef id: " << id
                     << " out: " << (i == _refs.end() ? " NONE " : i->second) << endl;

    if (i == _refs.end())
        return "";
    return i->second;
}

std::string CursorCache::getRefNS(long long id) const {
    verify(id);
    scoped_lock lk(_mutex);
    MapNormal::const_iterator i = _refsNS.find(id);

    LOG(_myLogLevel) << "CursorCache::getRefNs id: " << id
                     << " out: " << (i == _refsNS.end() ? " NONE " : i->second) << std::endl;

    if (i == _refsNS.end())
        return "";
    return i->second;
}


long long CursorCache::genId() {
    while (true) {
        scoped_lock lk(_mutex);

        long long x = Listener::getElapsedTimeMillis() << 32;
        x |= _random.nextInt32();

        if (x == 0)
            continue;

        if (x < 0)
            x *= -1;

        MapSharded::iterator i = _cursors.find(x);
        if (i != _cursors.end())
            continue;

        MapNormal::iterator j = _refs.find(x);
        if (j != _refs.end())
            continue;

        return x;
    }
}

void CursorCache::gotKillCursors(Message& m) {
    DbMessage dbmessage(m);
    int n = dbmessage.pullInt();

    if (n > 2000) {
        (n < 30000 ? warning() : error()) << "receivedKillCursors, n=" << n << endl;
    }

    uassert(13286, "sent 0 cursors to kill", n >= 1);
    uassert(13287, "too many cursors to kill", n < 30000);
    massert(18632,
            str::stream() << "bad kill cursors size: " << m.dataSize(),
            m.dataSize() == 8 + (8 * n));


    ConstDataCursor cursors(dbmessage.getArray(n));

    ClientBasic* client = ClientBasic::getCurrent();
    AuthorizationSession* authSession = client->getAuthorizationSession();
    for (int i = 0; i < n; i++) {
        long long id = cursors.readLEAndAdvance<int64_t>();
        LOG(_myLogLevel) << "CursorCache::gotKillCursors id: " << id << endl;

        if (!id) {
            warning() << " got cursor id of 0 to kill" << endl;
            continue;
        }

        string server;
        {
            scoped_lock lk(_mutex);

            MapSharded::iterator i = _cursors.find(id);
            if (i != _cursors.end()) {
                Status authorizationStatus =
                    authSession->checkAuthForKillCursors(NamespaceString(i->second->getNS()), id);
                audit::logKillCursorsAuthzCheck(
                    client,
                    NamespaceString(i->second->getNS()),
                    id,
                    authorizationStatus.isOK() ? ErrorCodes::OK : ErrorCodes::Unauthorized);
                if (authorizationStatus.isOK()) {
                    _cursorsMaxTimeMS.erase(i->second->getId());
                    _cursors.erase(i);
                }
                continue;
            }

            MapNormal::iterator refsIt = _refs.find(id);
            MapNormal::iterator refsNSIt = _refsNS.find(id);
            if (refsIt == _refs.end()) {
                warning() << "can't find cursor: " << id << endl;
                continue;
            }
            verify(refsNSIt != _refsNS.end());
            Status authorizationStatus =
                authSession->checkAuthForKillCursors(NamespaceString(refsNSIt->second), id);
            audit::logKillCursorsAuthzCheck(client,
                                            NamespaceString(refsNSIt->second),
                                            id,
                                            authorizationStatus.isOK() ? ErrorCodes::OK
                                                                       : ErrorCodes::Unauthorized);
            if (!authorizationStatus.isOK()) {
                continue;
            }
            server = refsIt->second;
            _refs.erase(refsIt);
            _refsNS.erase(refsNSIt);
            cursorStatsSingleTarget.decrement();
        }

        LOG(_myLogLevel) << "CursorCache::found gotKillCursors id: " << id << " server: " << server
                         << endl;

        verify(server.size());
        ScopedDbConnection conn(server);
        conn->killCursor(id);
        conn.done();
    }
}

void CursorCache::appendInfo(BSONObjBuilder& result) const {
    scoped_lock lk(_mutex);
    result.append("sharded", static_cast<int>(cursorStatsMultiTarget.get()));
    result.appendNumber("shardedEver", _shardedTotal);
    result.append("refs", static_cast<int>(cursorStatsSingleTarget.get()));
    result.append("totalOpen", static_cast<int>(cursorStatsTotalOpen.get()));
}

void CursorCache::doTimeouts() {
    long long now = Listener::getElapsedTimeMillis();
    scoped_lock lk(_mutex);
    for (MapSharded::iterator i = _cursors.begin(); i != _cursors.end(); ++i) {
        // Note: cursors with no timeout will always have an idleTime of 0
        long long idleFor = i->second->idleTime(now);
        if (idleFor < TIMEOUT) {
            continue;
        }
        log() << "killing old cursor " << i->second->getId() << " idle for: " << idleFor << "ms"
              << endl;  // TODO: make LOG(1)
        _cursorsMaxTimeMS.erase(i->second->getId());
        _cursors.erase(i);
        i = _cursors.begin();  // possible 2nd entry will get skipped, will get on next pass
        if (i == _cursors.end())
            break;
    }
}

CursorCache cursorCache;

const int CursorCache::_myLogLevel = 3;

class CursorTimeoutTask : public task::Task {
public:
    virtual string name() const {
        return "cursorTimeout";
    }
    virtual void doWork() {
        cursorCache.doTimeouts();
    }
};

void CursorCache::startTimeoutThread() {
    task::repeat(new CursorTimeoutTask, 4000);
}

class CmdCursorInfo : public Command {
public:
    CmdCursorInfo() : Command("cursorInfo", true) {}
    virtual bool slaveOk() const {
        return true;
    }
    virtual void help(stringstream& help) const {
        help << " example: { cursorInfo : 1 }";
    }
    virtual void addRequiredPrivileges(const std::string& dbname,
                                       const BSONObj& cmdObj,
                                       std::vector<Privilege>* out) {
        ActionSet actions;
        actions.addAction(ActionType::cursorInfo);
        out->push_back(Privilege(ResourcePattern::forClusterResource(), actions));
    }
    virtual bool isWriteCommandForConfigServer() const {
        return false;
    }
    bool run(OperationContext* txn,
             const string&,
             BSONObj& jsobj,
             int,
             string& errmsg,
             BSONObjBuilder& result,
             bool fromRepl) {
        cursorCache.appendInfo(result);
        if (jsobj["setTimeout"].isNumber())
            CursorCache::TIMEOUT = jsobj["setTimeout"].numberLong();
        return true;
    }
} cmdCursorInfo;
}
