/**
*    Copyright (C) 2009 10gen Inc.
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
*    must comply with the GNU Affero General Public License in all respects for
*    all of the code used other than as permitted herein. If you modify file(s)
*    with this exception, you may extend this exception to your version of the
*    file(s), but you are not obligated to do so. If you do not wish to do so,
*    delete this exception statement from your version. If you delete this
*    exception statement from all source files in the program, then also delete
*    it in the license file.
*/

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kDefault

#include "mongo/platform/basic.h"

#include "mongo/db/curop.h"

#include "mongo/base/counter.h"
#include "mongo/db/client.h"
#include "mongo/db/commands/server_status_metric.h"
#include "mongo/db/catalog/database.h"
#include "mongo/db/global_environment_experiment.h"
#include "mongo/db/stats/top.h"
#include "mongo/util/fail_point_service.h"
#include "mongo/util/log.h"

namespace mongo {

using std::string;

// Enabling the maxTimeAlwaysTimeOut fail point will cause any query or command run with a valid
// non-zero max time to fail immediately.  Any getmore operation on a cursor already created
// with a valid non-zero max time will also fail immediately.
//
// This fail point cannot be used with the maxTimeNeverTimeOut fail point.
MONGO_FP_DECLARE(maxTimeAlwaysTimeOut);

// Enabling the maxTimeNeverTimeOut fail point will cause the server to never time out any
// query, command, or getmore operation, regardless of whether a max time is set.
//
// This fail point cannot be used with the maxTimeAlwaysTimeOut fail point.
MONGO_FP_DECLARE(maxTimeNeverTimeOut);

// todo : move more here

CurOp::CurOp(Client* client, CurOp* wrapped) : _client(client), _wrapped(wrapped) {
    if (_wrapped)
        _client->_curOp = this;
    _start = 0;
    _active = false;
    _reset();
    _op = 0;
    _opNum = _nextOpNum.fetchAndAdd(1);
    _command = NULL;
}

void CurOp::_reset() {
    _isCommand = false;
    _dbprofile = 0;
    _end = 0;
    _maxTimeMicros = 0;
    _maxTimeTracker.reset();
    _message = "";
    _progressMeter.finished();
    _killPending.store(0);
    _numYields = 0;
    _expectedLatencyMs = 0;
}

void CurOp::reset() {
    _reset();
    _start = 0;
    _opNum = _nextOpNum.fetchAndAdd(1);
    _ns = "";
    _debug.reset();
    _query.reset();
    _active = true;  // this should be last for ui clarity
}

void CurOp::reset(const HostAndPort& remote, int op) {
    reset();
    if (_remote != remote) {
        // todo : _remote is not thread safe yet is used as such!
        _remote = remote;
    }
    _op = op;
}

ProgressMeter& CurOp::setMessage(const char* msg,
                                 std::string name,
                                 unsigned long long progressMeterTotal,
                                 int secondsBetween) {
    if (progressMeterTotal) {
        if (_progressMeter.isActive()) {
            error() << "old _message: " << _message << " new message:" << msg;
            verify(!_progressMeter.isActive());
        }
        _progressMeter.reset(progressMeterTotal, secondsBetween);
        _progressMeter.setName(name);
    } else {
        _progressMeter.finished();
    }
    _message = msg;
    return _progressMeter;
}

CurOp::~CurOp() {
    if (_wrapped) {
        boost::mutex::scoped_lock clientLock(Client::clientsMutex);
        _client->_curOp = _wrapped;
    }
    _client = 0;
}

void CurOp::setNS(const StringData& ns) {
    // _ns copies the data in the null-terminated ptr it's given
    _ns = ns;
}

void CurOp::ensureStarted() {
    if (_start == 0) {
        _start = curTimeMicros64();

        // If ensureStarted() is invoked after setMaxTimeMicros(), then time limit tracking will
        // start here.  This is because time limit tracking can only commence after the
        // operation is assigned a start time.
        if (_maxTimeMicros > 0) {
            _maxTimeTracker.setTimeLimit(_start, _maxTimeMicros);
        }
    }
}

void CurOp::enter(const char* ns, int dbProfileLevel) {
    ensureStarted();
    _ns = ns;
    _dbprofile = std::max(dbProfileLevel, _dbprofile);
}

void CurOp::recordGlobalTime(bool isWriteLocked, long long micros) const {
    string nsStr = _ns.toString();
    Top::global.record(nsStr, _op, isWriteLocked ? 1 : -1, micros, _isCommand);
}

void CurOp::reportState(BSONObjBuilder* builder) {
    builder->append("opid", _opNum);
    bool a = _active && _start;
    builder->append("active", a);

    if (a) {
        builder->append("secs_running", elapsedSeconds());
        builder->append("microsecs_running", static_cast<long long int>(elapsedMicros()));
    }

    builder->append("op", opToString(_op));

    // Fill out "ns" from our namespace member (and if it's not available, fall back to the
    // OpDebug namespace member).
    builder->append("ns", !_ns.empty() ? _ns.toString() : _debug.ns.toString());

    if (_op == dbInsert) {
        _query.append(*builder, "insert");
    } else {
        _query.append(*builder, "query");
    }

    if (!debug().planSummary.empty()) {
        builder->append("planSummary", debug().planSummary.toString());
    }

    if (!_remote.empty()) {
        builder->append("client", _remote.toString());
    }

    if (!_message.empty()) {
        if (_progressMeter.isActive()) {
            StringBuilder buf;
            buf << _message.toString() << " " << _progressMeter.toString();
            builder->append("msg", buf.str());
            BSONObjBuilder sub(builder->subobjStart("progress"));
            sub.appendNumber("done", (long long)_progressMeter.done());
            sub.appendNumber("total", (long long)_progressMeter.total());
            sub.done();
        } else {
            builder->append("msg", _message.toString());
        }
    }

    if (killPending())
        builder->append("killPending", true);

    builder->append("numYields", _numYields);
}

BSONObj CurOp::description() {
    BSONObjBuilder bob;
    bool a = _active && _start;
    bob.append("active", a);
    bob.append("op", opToString(_op));

    // Fill out "ns" from our namespace member (and if it's not available, fall back to the
    // OpDebug namespace member).
    bob.append("ns", !_ns.empty() ? _ns.toString() : _debug.ns.toString());

    if (_op == dbInsert) {
        _query.append(bob, "insert");
    } else {
        _query.append(bob, "query");
    }
    if (killPending())
        bob.append("killPending", true);
    return bob.obj();
}

void CurOp::kill() {
    _killPending.store(1);
}

void CurOp::setMaxTimeMicros(uint64_t maxTimeMicros) {
    _maxTimeMicros = maxTimeMicros;

    if (_maxTimeMicros == 0) {
        // 0 is "allow to run indefinitely".
        return;
    }

    // If the operation has a start time, then enable the tracker.
    //
    // If the operation has no start time yet, then ensureStarted() will take responsibility for
    // enabling the tracker.
    if (isStarted()) {
        _maxTimeTracker.setTimeLimit(startTime(), _maxTimeMicros);
    }
}

bool CurOp::maxTimeHasExpired() {
    if (MONGO_FAIL_POINT(maxTimeNeverTimeOut)) {
        return false;
    }
    if (_maxTimeMicros > 0 && MONGO_FAIL_POINT(maxTimeAlwaysTimeOut)) {
        return true;
    }
    return _maxTimeTracker.checkTimeLimit();
}

uint64_t CurOp::getRemainingMaxTimeMicros() const {
    return _maxTimeTracker.getRemainingMicros();
}

AtomicUInt32 CurOp::_nextOpNum;

static Counter64 returnedCounter;
static Counter64 insertedCounter;
static Counter64 updatedCounter;
static Counter64 deletedCounter;
static Counter64 scannedCounter;
static Counter64 scannedObjectCounter;

static ServerStatusMetricField<Counter64> displayReturned("document.returned", &returnedCounter);
static ServerStatusMetricField<Counter64> displayUpdated("document.updated", &updatedCounter);
static ServerStatusMetricField<Counter64> displayInserted("document.inserted", &insertedCounter);
static ServerStatusMetricField<Counter64> displayDeleted("document.deleted", &deletedCounter);
static ServerStatusMetricField<Counter64> displayScanned("queryExecutor.scanned", &scannedCounter);
static ServerStatusMetricField<Counter64> displayScannedObjects("queryExecutor.scannedObjects",
                                                                &scannedObjectCounter);

static Counter64 idhackCounter;
static Counter64 scanAndOrderCounter;
static Counter64 fastmodCounter;
static Counter64 writeConflictsCounter;

static ServerStatusMetricField<Counter64> displayIdhack("operation.idhack", &idhackCounter);
static ServerStatusMetricField<Counter64> displayScanAndOrder("operation.scanAndOrder",
                                                              &scanAndOrderCounter);
static ServerStatusMetricField<Counter64> displayFastMod("operation.fastmod", &fastmodCounter);
static ServerStatusMetricField<Counter64> displayWriteConflicts("operation.writeConflicts",
                                                                &writeConflictsCounter);

void OpDebug::recordStats() {
    if (nreturned > 0)
        returnedCounter.increment(nreturned);
    if (ninserted > 0)
        insertedCounter.increment(ninserted);
    if (nMatched > 0)
        updatedCounter.increment(nMatched);
    if (ndeleted > 0)
        deletedCounter.increment(ndeleted);
    if (nscanned > 0)
        scannedCounter.increment(nscanned);
    if (nscannedObjects > 0)
        scannedObjectCounter.increment(nscannedObjects);

    if (idhack)
        idhackCounter.increment();
    if (scanAndOrder)
        scanAndOrderCounter.increment();
    if (fastmod)
        fastmodCounter.increment();
    if (writeConflicts)
        writeConflictsCounter.increment(writeConflicts);
}

CurOp::MaxTimeTracker::MaxTimeTracker() {
    reset();
}

void CurOp::MaxTimeTracker::reset() {
    _enabled = false;
    _targetEpochMicros = 0;
    _approxTargetServerMillis = 0;
}

void CurOp::MaxTimeTracker::setTimeLimit(uint64_t startEpochMicros, uint64_t durationMicros) {
    dassert(durationMicros != 0);

    _enabled = true;

    _targetEpochMicros = startEpochMicros + durationMicros;

    uint64_t now = curTimeMicros64();
    // If our accurate time source thinks time is not up yet, calculate the next target for
    // our approximate time source.
    if (_targetEpochMicros > now) {
        _approxTargetServerMillis = Listener::getElapsedTimeMillis() +
            static_cast<int64_t>((_targetEpochMicros - now) / 1000);
    }
    // Otherwise, set our approximate time source target such that it thinks time is already
    // up.
    else {
        _approxTargetServerMillis = Listener::getElapsedTimeMillis();
    }
}

bool CurOp::MaxTimeTracker::checkTimeLimit() {
    if (!_enabled) {
        return false;
    }

    // Does our approximate time source think time is not up yet?  If so, return early.
    if (_approxTargetServerMillis > Listener::getElapsedTimeMillis()) {
        return false;
    }

    uint64_t now = curTimeMicros64();
    // Does our accurate time source think time is not up yet?  If so, readjust the target for
    // our approximate time source and return early.
    if (_targetEpochMicros > now) {
        _approxTargetServerMillis = Listener::getElapsedTimeMillis() +
            static_cast<int64_t>((_targetEpochMicros - now) / 1000);
        return false;
    }

    // Otherwise, time is up.
    return true;
}

uint64_t CurOp::MaxTimeTracker::getRemainingMicros() const {
    if (!_enabled) {
        // 0 is "allow to run indefinitely".
        return 0;
    }

    // Does our accurate time source think time is up?  If so, claim there is 1 microsecond
    // left for this operation.
    uint64_t now = curTimeMicros64();
    if (_targetEpochMicros <= now) {
        return 1;
    }

    // Otherwise, calculate remaining time.
    return _targetEpochMicros - now;
}
}
