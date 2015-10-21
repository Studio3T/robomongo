/**
*    Copyright (C) 2008-2014 MongoDB Inc.
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

/* Collections we use:

   local.sources         - indicates what sources we pull from as a "slave", and the last update of
                            each
   local.oplog.$main     - our op log as "master"
   local.dbinfo.<dbname> - no longer used???
   local.pair.startup    - [deprecated] can contain a special value indicating for a pair that we
                           have the master copy. used when replacing other half of the pair which
                           has permanently failed.
   local.pair.sync       - [deprecated] { initialsynccomplete: 1 }
*/

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kReplication

#include "mongo/platform/basic.h"

#include "mongo/db/repl/master_slave.h"

#include <iostream>
#include <pcrecpp.h>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>

#include "mongo/db/auth/authorization_manager.h"
#include "mongo/db/auth/authorization_session.h"
#include "mongo/db/catalog/database_catalog_entry.h"
#include "mongo/db/catalog/database_holder.h"
#include "mongo/db/cloner.h"
#include "mongo/db/commands.h"
#include "mongo/db/dbdirectclient.h"
#include "mongo/db/dbhelpers.h"
#include "mongo/db/ops/update.h"
#include "mongo/db/query/internal_plans.h"
#include "mongo/db/repl/oplog.h"
#include "mongo/db/repl/replication_coordinator_global.h"
#include "mongo/db/repl/sync.h"
#include "mongo/db/server_parameters.h"
#include "mongo/db/operation_context_impl.h"
#include "mongo/db/storage_options.h"
#include "mongo/util/concurrency/thread_pool.h"
#include "mongo/util/exit.h"
#include "mongo/util/log.h"

using boost::scoped_ptr;
using std::auto_ptr;
using std::cout;
using std::endl;
using std::max;
using std::min;
using std::set;
using std::stringstream;
using std::vector;

namespace mongo {
namespace repl {

void pretouchOperation(OperationContext* txn, const BSONObj& op);
void pretouchN(vector<BSONObj>&, unsigned a, unsigned b);

/* if 1 sync() is running */
volatile int syncing = 0;
volatile int relinquishSyncingSome = 0;

static time_t lastForcedResync = 0;

/* output by the web console */
const char* replInfo = "";
struct ReplInfo {
    ReplInfo(const char* msg) {
        replInfo = msg;
    }
    ~ReplInfo() {
        replInfo = "?";
    }
};


ReplSource::ReplSource(OperationContext* txn) {
    nClonedThisPass = 0;
    ensureMe(txn);
}

ReplSource::ReplSource(OperationContext* txn, BSONObj o) : nClonedThisPass(0) {
    only = o.getStringField("only");
    hostName = o.getStringField("host");
    _sourceName = o.getStringField("source");
    uassert(10118, "'host' field not set in sources collection object", !hostName.empty());
    uassert(10119, "only source='main' allowed for now with replication", sourceName() == "main");
    BSONElement e = o.getField("syncedTo");
    if (!e.eoo()) {
        uassert(
            10120, "bad sources 'syncedTo' field value", e.type() == Date || e.type() == Timestamp);
        OpTime tmp(e.date());
        syncedTo = tmp;
    }

    BSONObj dbsObj = o.getObjectField("dbsNextPass");
    if (!dbsObj.isEmpty()) {
        BSONObjIterator i(dbsObj);
        while (1) {
            BSONElement e = i.next();
            if (e.eoo())
                break;
            addDbNextPass.insert(e.fieldName());
        }
    }

    dbsObj = o.getObjectField("incompleteCloneDbs");
    if (!dbsObj.isEmpty()) {
        BSONObjIterator i(dbsObj);
        while (1) {
            BSONElement e = i.next();
            if (e.eoo())
                break;
            incompleteCloneDbs.insert(e.fieldName());
        }
    }
    ensureMe(txn);
}

/* Turn our C++ Source object into a BSONObj */
BSONObj ReplSource::jsobj() {
    BSONObjBuilder b;
    b.append("host", hostName);
    b.append("source", sourceName());
    if (!only.empty())
        b.append("only", only);
    if (!syncedTo.isNull())
        b.appendTimestamp("syncedTo", syncedTo.asDate());

    BSONObjBuilder dbsNextPassBuilder;
    int n = 0;
    for (set<string>::iterator i = addDbNextPass.begin(); i != addDbNextPass.end(); i++) {
        n++;
        dbsNextPassBuilder.appendBool(*i, 1);
    }
    if (n)
        b.append("dbsNextPass", dbsNextPassBuilder.done());

    BSONObjBuilder incompleteCloneDbsBuilder;
    n = 0;
    for (set<string>::iterator i = incompleteCloneDbs.begin(); i != incompleteCloneDbs.end(); i++) {
        n++;
        incompleteCloneDbsBuilder.appendBool(*i, 1);
    }
    if (n)
        b.append("incompleteCloneDbs", incompleteCloneDbsBuilder.done());

    return b.obj();
}

void ReplSource::ensureMe(OperationContext* txn) {
    string myname = getHostName();

    // local.me is an identifier for a server for getLastError w:2+
    bool exists = Helpers::getSingleton(txn, "local.me", _me);

    if (!exists || !_me.hasField("host") || _me["host"].String() != myname) {
        ScopedTransaction transaction(txn, MODE_IX);
        Lock::DBLock dblk(txn->lockState(), "local", MODE_X);
        WriteUnitOfWork wunit(txn);
        // clean out local.me
        Helpers::emptyCollection(txn, "local.me");

        // repopulate
        BSONObjBuilder b;
        b.appendOID("_id", 0, true);
        b.append("host", myname);
        _me = b.obj();
        Helpers::putSingleton(txn, "local.me", _me);
        wunit.commit();
    }
    _me = _me.getOwned();
}

void ReplSource::save(OperationContext* txn) {
    BSONObjBuilder b;
    verify(!hostName.empty());
    b.append("host", hostName);
    // todo: finish allowing multiple source configs.
    // this line doesn't work right when source is null, if that is allowed as it is now:
    // b.append("source", _sourceName);
    BSONObj pattern = b.done();

    BSONObj o = jsobj();
    LOG(1) << "Saving repl source: " << o << endl;

    {
        OpDebug debug;

        Client::Context ctx(txn, "local.sources");

        const NamespaceString requestNs("local.sources");
        UpdateRequest request(requestNs);

        request.setQuery(pattern);
        request.setUpdates(o);
        request.setUpsert();

        UpdateResult res = update(txn, ctx.db(), request, &debug);

        verify(!res.modifiers);
        verify(res.numMatched == 1);
    }
}

static void addSourceToList(OperationContext* txn,
                            ReplSource::SourceVector& v,
                            ReplSource& s,
                            ReplSource::SourceVector& old) {
    if (!s.syncedTo.isNull()) {  // Don't reuse old ReplSource if there was a forced resync.
        for (ReplSource::SourceVector::iterator i = old.begin(); i != old.end();) {
            if (s == **i) {
                v.push_back(*i);
                old.erase(i);
                return;
            }
            i++;
        }
    }

    v.push_back(boost::shared_ptr<ReplSource>(new ReplSource(s)));
}

/* we reuse our existing objects so that we can keep our existing connection
   and cursor in effect.
*/
void ReplSource::loadAll(OperationContext* txn, SourceVector& v) {
    const char* localSources = "local.sources";
    Client::Context ctx(txn, localSources);
    SourceVector old = v;
    v.clear();

    const ReplSettings& replSettings = getGlobalReplicationCoordinator()->getSettings();
    if (!replSettings.source.empty()) {
        // --source <host> specified.
        // check that no items are in sources other than that
        // add if missing
        int n = 0;
        auto_ptr<PlanExecutor> exec(InternalPlanner::collectionScan(
            txn, localSources, ctx.db()->getCollection(localSources)));
        BSONObj obj;
        PlanExecutor::ExecState state;
        while (PlanExecutor::ADVANCED == (state = exec->getNext(&obj, NULL))) {
            n++;
            ReplSource tmp(txn, obj);
            if (tmp.hostName != replSettings.source) {
                log() << "repl: --source " << replSettings.source << " != " << tmp.hostName
                      << " from local.sources collection" << endl;
                log() << "repl: for instructions on changing this slave's source, see:" << endl;
                log() << "http://dochub.mongodb.org/core/masterslave" << endl;
                log() << "repl: terminating mongod after 30 seconds" << endl;
                sleepsecs(30);
                dbexit(EXIT_REPLICATION_ERROR);
            }
            if (tmp.only != replSettings.only) {
                log() << "--only " << replSettings.only << " != " << tmp.only
                      << " from local.sources collection" << endl;
                log() << "terminating after 30 seconds" << endl;
                sleepsecs(30);
                dbexit(EXIT_REPLICATION_ERROR);
            }
        }
        uassert(17065, "Internal error reading from local.sources", PlanExecutor::IS_EOF == state);
        uassert(10002, "local.sources collection corrupt?", n < 2);
        if (n == 0) {
            // source missing.  add.
            ReplSource s(txn);
            s.hostName = replSettings.source;
            s.only = replSettings.only;
            s.save(txn);
        }
    } else {
        try {
            massert(10384, "--only requires use of --source", replSettings.only.empty());
        } catch (...) {
            dbexit(EXIT_BADOPTIONS);
        }
    }

    auto_ptr<PlanExecutor> exec(
        InternalPlanner::collectionScan(txn, localSources, ctx.db()->getCollection(localSources)));
    BSONObj obj;
    PlanExecutor::ExecState state;
    while (PlanExecutor::ADVANCED == (state = exec->getNext(&obj, NULL))) {
        ReplSource tmp(txn, obj);
        if (tmp.syncedTo.isNull()) {
            DBDirectClient c(txn);
            BSONObj op = c.findOne("local.oplog.$main",
                                   QUERY("op" << NE << "n").sort(BSON("$natural" << -1)));
            if (!op.isEmpty()) {
                tmp.syncedTo = op["ts"].date();
            }
        }
        addSourceToList(txn, v, tmp, old);
    }
    uassert(17066, "Internal error reading from local.sources", PlanExecutor::IS_EOF == state);
}

bool ReplSource::throttledForceResyncDead(OperationContext* txn, const char* requester) {
    if (time(0) - lastForcedResync > 600) {
        forceResyncDead(txn, requester);
        lastForcedResync = time(0);
        return true;
    }
    return false;
}

void ReplSource::forceResyncDead(OperationContext* txn, const char* requester) {
    if (!replAllDead)
        return;
    SourceVector sources;
    ReplSource::loadAll(txn, sources);
    for (SourceVector::iterator i = sources.begin(); i != sources.end(); ++i) {
        log() << requester << " forcing resync from " << (*i)->hostName << endl;
        (*i)->forceResync(txn, requester);
    }
    replAllDead = 0;
}

bool replHandshake(DBClientConnection* conn, const OID& myRID) {
    string myname = getHostName();

    BSONObjBuilder cmd;
    cmd.append("handshake", myRID);

    BSONObj res;
    bool ok = conn->runCommand("admin", cmd.obj(), res);
    // ignoring for now on purpose for older versions
    LOG(ok ? 1 : 0) << "replHandshake res not: " << ok << " res: " << res << endl;
    return true;
}

bool ReplSource::_connect(OplogReader* reader, const HostAndPort& host, const OID& myRID) {
    if (reader->conn()) {
        return true;
    }

    if (!reader->connect(host)) {
        return false;
    }

    if (!replHandshake(reader->conn(), myRID)) {
        return false;
    }

    return true;
}


void ReplSource::forceResync(OperationContext* txn, const char* requester) {
    BSONObj info;
    {
        // This is always a GlobalWrite lock (so no ns/db used from the context)
        invariant(txn->lockState()->isW());
        Lock::TempRelease tempRelease(txn->lockState());

        if (!_connect(&oplogReader,
                      HostAndPort(hostName),
                      getGlobalReplicationCoordinator()->getMyRID())) {
            msgassertedNoTrace(14051, "unable to connect to resync");
        }
        /* todo use getDatabaseNames() method here */
        bool ok = oplogReader.conn()->runCommand(
            "admin", BSON("listDatabases" << 1), info, QueryOption_SlaveOk);
        massert(10385, "Unable to get database list", ok);
    }

    BSONObjIterator i(info.getField("databases").embeddedObject());
    while (i.moreWithEOO()) {
        BSONElement e = i.next();
        if (e.eoo())
            break;
        string name = e.embeddedObject().getField("name").valuestr();
        if (!e.embeddedObject().getBoolField("empty")) {
            if (name != "local") {
                if (only.empty() || only == name) {
                    resyncDrop(txn, name);
                }
            }
        }
    }
    syncedTo = OpTime();
    addDbNextPass.clear();
    save(txn);
}

void ReplSource::resyncDrop(OperationContext* txn, const string& db) {
    log() << "resync: dropping database " << db;
    Client::Context ctx(txn, db);
    dropDatabase(txn, ctx.db());
}

/* grab initial copy of a database from the master */
void ReplSource::resync(OperationContext* txn, const std::string& dbName) {
    const std::string db(dbName);  // need local copy of the name, we're dropping the original
    resyncDrop(txn, db);

    {
        log() << "resync: cloning database " << db << " to get an initial copy" << endl;
        ReplInfo r("resync: cloning a database");
        string errmsg;
        int errCode = 0;
        CloneOptions cloneOptions;
        cloneOptions.fromDB = db;
        cloneOptions.logForRepl = false;
        cloneOptions.slaveOk = true;
        cloneOptions.useReplAuth = true;
        cloneOptions.snapshot = true;
        cloneOptions.mayYield = true;
        cloneOptions.mayBeInterrupted = false;

        Cloner cloner;
        bool ok = cloner.go(txn, db, hostName.c_str(), cloneOptions, NULL, errmsg, &errCode);

        if (!ok) {
            if (errCode == DatabaseDifferCaseCode) {
                resyncDrop(txn, db);
                log() << "resync: database " << db
                      << " not valid on the master due to a name conflict, dropping." << endl;
                return;
            } else {
                log() << "resync of " << db << " from " << hostName << " failed " << errmsg << endl;
                throw SyncException();
            }
        }
    }

    log() << "resync: done with initial clone for db: " << db << endl;

    return;
}

static DatabaseIgnorer ___databaseIgnorer;

void DatabaseIgnorer::doIgnoreUntilAfter(const string& db, const OpTime& futureOplogTime) {
    if (futureOplogTime > _ignores[db]) {
        _ignores[db] = futureOplogTime;
    }
}

bool DatabaseIgnorer::ignoreAt(const string& db, const OpTime& currentOplogTime) {
    if (_ignores[db].isNull()) {
        return false;
    }
    if (_ignores[db] >= currentOplogTime) {
        return true;
    } else {
        // The ignore state has expired, so clear it.
        _ignores.erase(db);
        return false;
    }
}

bool ReplSource::handleDuplicateDbName(OperationContext* txn,
                                       const BSONObj& op,
                                       const char* ns,
                                       const char* db) {
    // We are already locked at this point
    if (dbHolder().get(txn, ns) != NULL) {
        // Database is already present.
        return true;
    }
    BSONElement ts = op.getField("ts");
    if ((ts.type() == Date || ts.type() == Timestamp) &&
        ___databaseIgnorer.ignoreAt(db, ts.date())) {
        // Database is ignored due to a previous indication that it is
        // missing from master after optime "ts".
        return false;
    }
    if (Database::duplicateUncasedName(db).empty()) {
        // No duplicate database names are present.
        return true;
    }

    OpTime lastTime;
    bool dbOk = false;
    {
        // This is always a GlobalWrite lock (so no ns/db used from the context)
        invariant(txn->lockState()->isW());
        Lock::TempRelease(txn->lockState());

        // We always log an operation after executing it (never before), so
        // a database list will always be valid as of an oplog entry generated
        // before it was retrieved.

        BSONObj last =
            oplogReader.findOne(this->ns().c_str(), Query().sort(BSON("$natural" << -1)));
        if (!last.isEmpty()) {
            BSONElement ts = last.getField("ts");
            massert(
                14032, "Invalid 'ts' in remote log", ts.type() == Date || ts.type() == Timestamp);
            lastTime = OpTime(ts.date());
        }

        BSONObj info;
        bool ok = oplogReader.conn()->runCommand("admin", BSON("listDatabases" << 1), info);
        massert(14033, "Unable to get database list", ok);
        BSONObjIterator i(info.getField("databases").embeddedObject());
        while (i.more()) {
            BSONElement e = i.next();

            const char* name = e.embeddedObject().getField("name").valuestr();
            if (strcasecmp(name, db) != 0)
                continue;

            if (strcmp(name, db) == 0) {
                // The db exists on master, still need to check that no conflicts exist there.
                dbOk = true;
                continue;
            }

            // The master has a db name that conflicts with the requested name.
            dbOk = false;
            break;
        }
    }

    if (!dbOk) {
        ___databaseIgnorer.doIgnoreUntilAfter(db, lastTime);
        incompleteCloneDbs.erase(db);
        addDbNextPass.erase(db);
        return false;
    }

    // Check for duplicates again, since we released the lock above.
    set<string> duplicates;
    Database::duplicateUncasedName(db, &duplicates);

    // The database is present on the master and no conflicting databases
    // are present on the master.  Drop any local conflicts.
    for (set<string>::const_iterator i = duplicates.begin(); i != duplicates.end(); ++i) {
        ___databaseIgnorer.doIgnoreUntilAfter(*i, lastTime);
        incompleteCloneDbs.erase(*i);
        addDbNextPass.erase(*i);

        Client::Context ctx(txn, *i);
        dropDatabase(txn, ctx.db());
    }

    massert(14034,
            "Duplicate database names present after attempting to delete duplicates",
            Database::duplicateUncasedName(db).empty());
    return true;
}

void ReplSource::applyOperation(OperationContext* txn, Database* db, const BSONObj& op) {
    try {
        bool failedUpdate = applyOperation_inlock(txn, db, op);
        if (failedUpdate) {
            Sync sync(hostName);
            if (sync.shouldRetry(txn, op)) {
                uassert(15914,
                        "Failure retrying initial sync update",
                        !applyOperation_inlock(txn, db, op));
            }
        }
    } catch (UserException& e) {
        log() << "sync: caught user assertion " << e << " while applying op: " << op << endl;
        ;
    } catch (DBException& e) {
        log() << "sync: caught db exception " << e << " while applying op: " << op << endl;
        ;
    }
}

/* local.$oplog.main is of the form:
     { ts: ..., op: <optype>, ns: ..., o: <obj> , o2: <extraobj>, b: <boolflag> }
     ...
   see logOp() comments.

   @param alreadyLocked caller already put us in write lock if true
*/
void ReplSource::_sync_pullOpLog_applyOperation(OperationContext* txn,
                                                BSONObj& op,
                                                bool alreadyLocked) {
    LOG(6) << "processing op: " << op << endl;

    if (op.getStringField("op")[0] == 'n')
        return;

    char clientName[MaxDatabaseNameLen];
    const char* ns = op.getStringField("ns");
    nsToDatabase(ns, clientName);

    if (*ns == '.') {
        log() << "skipping bad op in oplog: " << op.toString() << endl;
        return;
    } else if (*ns == 0) {
        /*if( op.getStringField("op")[0] != 'n' )*/ {
            log() << "halting replication, bad op in oplog:\n  " << op.toString() << endl;
            replAllDead = "bad object in oplog";
            throw SyncException();
        }
        // ns = "local.system.x";
        // nsToDatabase(ns, clientName);
    }

    if (!only.empty() && only != clientName)
        return;

    const ReplSettings& replSettings = getGlobalReplicationCoordinator()->getSettings();
    if (replSettings.pretouch && !alreadyLocked /*doesn't make sense if in write lock already*/) {
        if (replSettings.pretouch > 1) {
            /* note: this is bad - should be put in ReplSource.  but this is first test... */
            static int countdown;
            verify(countdown >= 0);
            if (countdown > 0) {
                countdown--;  // was pretouched on a prev pass
            } else {
                const int m = 4;
                if (tp.get() == 0) {
                    int nthr = min(8, replSettings.pretouch);
                    nthr = max(nthr, 1);
                    tp.reset(new ThreadPool(nthr));
                }
                vector<BSONObj> v;
                oplogReader.peek(v, replSettings.pretouch);
                unsigned a = 0;
                while (1) {
                    if (a >= v.size())
                        break;
                    unsigned b = a + m - 1;  // v[a..b]
                    if (b >= v.size())
                        b = v.size() - 1;
                    tp->schedule(pretouchN, v, a, b);
                    DEV cout << "pretouch task: " << a << ".." << b << endl;
                    a += m;
                }
                // we do one too...
                pretouchOperation(txn, op);
                tp->join();
                countdown = v.size();
            }
        } else {
            pretouchOperation(txn, op);
        }
    }

    scoped_ptr<Lock::GlobalWrite> lk(alreadyLocked ? 0 : new Lock::GlobalWrite(txn->lockState()));

    if (replAllDead) {
        // hmmm why is this check here and not at top of this function? does it get set between top
        // and here?
        log() << "replAllDead, throwing SyncException: " << replAllDead << endl;
        throw SyncException();
    }

    if (!handleDuplicateDbName(txn, op, ns, clientName)) {
        return;
    }

    // This code executes on the slaves only, so it doesn't need to be sharding-aware since
    // mongos will not send requests there. That's why the last argument is false (do not do
    // version checking).
    Client::Context ctx(txn, ns, false);
    ctx.getClient()->curop()->reset();

    bool empty = !ctx.db()->getDatabaseCatalogEntry()->hasUserData();
    bool incompleteClone = incompleteCloneDbs.count(clientName) != 0;

    LOG(6) << "ns: " << ns << ", justCreated: " << ctx.justCreated() << ", empty: " << empty
           << ", incompleteClone: " << incompleteClone << endl;

    // always apply admin command command
    // this is a bit hacky -- the semantics of replication/commands aren't well specified
    if (strcmp(clientName, "admin") == 0 && *op.getStringField("op") == 'c') {
        applyOperation(txn, ctx.db(), op);
        return;
    }

    if (ctx.justCreated() || empty || incompleteClone) {
        // we must add to incomplete list now that setClient has been called
        incompleteCloneDbs.insert(clientName);
        if (nClonedThisPass) {
            /* we only clone one database per pass, even if a lot need done.  This helps us
             avoid overflowing the master's transaction log by doing too much work before going
             back to read more transactions. (Imagine a scenario of slave startup where we try to
             clone 100 databases in one pass.)
             */
            addDbNextPass.insert(clientName);
        } else {
            if (incompleteClone) {
                log() << "An earlier initial clone of '" << clientName
                      << "' did not complete, now resyncing." << endl;
            }
            save(txn);
            Client::Context ctx(txn, ns);
            nClonedThisPass++;
            resync(txn, ctx.db()->name());
            addDbNextPass.erase(clientName);
            incompleteCloneDbs.erase(clientName);
        }
        save(txn);
    } else {
        applyOperation(txn, ctx.db(), op);
        addDbNextPass.erase(clientName);
    }
}

void ReplSource::syncToTailOfRemoteLog() {
    string _ns = ns();
    BSONObjBuilder b;
    if (!only.empty()) {
        b.appendRegex("ns", string("^") + pcrecpp::RE::QuoteMeta(only));
    }
    BSONObj last = oplogReader.findOne(_ns.c_str(), Query(b.done()).sort(BSON("$natural" << -1)));
    if (!last.isEmpty()) {
        BSONElement ts = last.getField("ts");
        massert(10386,
                "non Date ts found: " + last.toString(),
                ts.type() == Date || ts.type() == Timestamp);
        syncedTo = OpTime(ts.date());
    }
}

class ReplApplyBatchSize : public ServerParameter {
public:
    ReplApplyBatchSize()
        : ServerParameter(ServerParameterSet::getGlobal(), "replApplyBatchSize"), _value(1) {}

    int get() const {
        return _value;
    }

    virtual void append(OperationContext* txn, BSONObjBuilder& b, const string& name) {
        b.append(name, _value);
    }

    virtual Status set(const BSONElement& newValuElement) {
        return set(newValuElement.numberInt());
    }

    virtual Status set(int b) {
        if (b < 1 || b > 1024) {
            return Status(ErrorCodes::BadValue, "replApplyBatchSize has to be >= 1 and < 1024");
        }

        const ReplSettings& replSettings = getGlobalReplicationCoordinator()->getSettings();
        if (replSettings.slavedelay != 0 && b > 1) {
            return Status(ErrorCodes::BadValue, "can't use a batch size > 1 with slavedelay");
        }
        if (!replSettings.slave) {
            return Status(ErrorCodes::BadValue,
                          "can't set replApplyBatchSize on a non-slave machine");
        }

        _value = b;
        return Status::OK();
    }

    virtual Status setFromString(const string& str) {
        return set(atoi(str.c_str()));
    }

    int _value;

} replApplyBatchSize;

/* slave: pull some data from the master's oplog
   note: not yet in db mutex at this point.
   @return -1 error
           0 ok, don't sleep
           1 ok, sleep
*/
int ReplSource::_sync_pullOpLog(OperationContext* txn, int& nApplied) {
    int okResultCode = 1;
    string ns = string("local.oplog.$") + sourceName();
    LOG(2) << "repl: sync_pullOpLog " << ns << " syncedTo:" << syncedTo.toStringLong() << '\n';

    bool tailing = true;
    oplogReader.tailCheck();

    bool initial = syncedTo.isNull();

    if (!oplogReader.haveCursor() || initial) {
        if (initial) {
            // Important to grab last oplog timestamp before listing databases.
            syncToTailOfRemoteLog();
            BSONObj info;
            bool ok = oplogReader.conn()->runCommand("admin", BSON("listDatabases" << 1), info);
            massert(10389, "Unable to get database list", ok);
            BSONObjIterator i(info.getField("databases").embeddedObject());
            while (i.moreWithEOO()) {
                BSONElement e = i.next();
                if (e.eoo())
                    break;
                string name = e.embeddedObject().getField("name").valuestr();
                if (!e.embeddedObject().getBoolField("empty")) {
                    if (name != "local") {
                        if (only.empty() || only == name) {
                            LOG(2) << "adding to 'addDbNextPass': " << name << endl;
                            addDbNextPass.insert(name);
                        }
                    }
                }
            }
            // obviously global isn't ideal, but non-repl set is old so
            // keeping it simple
            ScopedTransaction transaction(txn, MODE_X);
            Lock::GlobalWrite lk(txn->lockState());
            save(txn);
        }

        BSONObjBuilder gte;
        gte.appendTimestamp("$gte", syncedTo.asDate());
        BSONObjBuilder query;
        query.append("ts", gte.done());
        if (!only.empty()) {
            // note we may here skip a LOT of data table scanning, a lot of work for the master.
            // maybe append "\\." here?
            query.appendRegex("ns", string("^") + pcrecpp::RE::QuoteMeta(only));
        }
        BSONObj queryObj = query.done();
        // e.g. queryObj = { ts: { $gte: syncedTo } }

        oplogReader.tailingQuery(ns.c_str(), queryObj);
        tailing = false;
    } else {
        LOG(2) << "repl: tailing=true\n";
    }

    if (!oplogReader.haveCursor()) {
        log() << "repl: dbclient::query returns null (conn closed?)" << endl;
        oplogReader.resetConnection();
        return -1;
    }

    // show any deferred database creates from a previous pass
    {
        set<string>::iterator i = addDbNextPass.begin();
        if (i != addDbNextPass.end()) {
            BSONObjBuilder b;
            b.append("ns", *i + '.');
            b.append("op", "db");
            BSONObj op = b.done();
            _sync_pullOpLog_applyOperation(txn, op, false);
        }
    }

    if (!oplogReader.more()) {
        if (tailing) {
            LOG(2) << "repl: tailing & no new activity\n";
            okResultCode = 0;  // don't sleep

        } else {
            log() << "repl:   " << ns << " oplog is empty" << endl;
        }
        {
            ScopedTransaction transaction(txn, MODE_X);
            Lock::GlobalWrite lk(txn->lockState());
            save(txn);
        }
        return okResultCode;
    }

    OpTime nextOpTime;
    {
        BSONObj op = oplogReader.next();
        BSONElement ts = op.getField("ts");
        if (ts.type() != Date && ts.type() != Timestamp) {
            string err = op.getStringField("$err");
            if (!err.empty()) {
                // 13051 is "tailable cursor requested on non capped collection"
                if (op.getIntField("code") == 13051) {
                    log() << "trying to slave off of a non-master" << '\n';
                    massert(13344, "trying to slave off of a non-master", false);
                } else {
                    log() << "repl: $err reading remote oplog: " + err << '\n';
                    massert(10390, "got $err reading remote oplog", false);
                }
            } else {
                log() << "repl: bad object read from remote oplog: " << op.toString() << '\n';
                massert(10391, "repl: bad object read from remote oplog", false);
            }
        }

        nextOpTime = OpTime(ts.date());
        LOG(2) << "repl: first op time received: " << nextOpTime.toString() << '\n';
        if (initial) {
            LOG(1) << "repl:   initial run\n";
        }
        if (tailing) {
            if (!(syncedTo < nextOpTime)) {
                log() << "repl ASSERTION failed : syncedTo < nextOpTime" << endl;
                log() << "repl syncTo:     " << syncedTo.toStringLong() << endl;
                log() << "repl nextOpTime: " << nextOpTime.toStringLong() << endl;
                verify(false);
            }
            oplogReader.putBack(op);          // op will be processed in the loop below
            nextOpTime = OpTime();            // will reread the op below
        } else if (nextOpTime != syncedTo) {  // didn't get what we queried for - error
            log() << "repl:   nextOpTime " << nextOpTime.toStringLong() << ' '
                  << ((nextOpTime < syncedTo) ? "<??" : ">") << " syncedTo "
                  << syncedTo.toStringLong() << '\n'
                  << "repl:   time diff: " << (nextOpTime.getSecs() - syncedTo.getSecs()) << "sec\n"
                  << "repl:   tailing: " << tailing << '\n'
                  << "repl:   data too stale, halting replication" << endl;
            replInfo = replAllDead = "data too stale halted replication";
            verify(syncedTo < nextOpTime);
            throw SyncException();
        } else {
            /* t == syncedTo, so the first op was applied previously or it is the first op of
             * initial query and need not be applied. */
        }
    }

    // apply operations
    {
        int n = 0;
        time_t saveLast = time(0);
        while (1) {
            // we need "&& n" to assure we actually process at least one op to get a sync
            // point recorded in the first place.
            const bool moreInitialSyncsPending = !addDbNextPass.empty() && n;

            if (moreInitialSyncsPending || !oplogReader.more()) {
                ScopedTransaction transaction(txn, MODE_X);
                Lock::GlobalWrite lk(txn->lockState());

                if (tailing) {
                    okResultCode = 0;  // don't sleep
                }

                syncedTo = nextOpTime;
                save(txn);  // note how far we are synced up to now
                nApplied = n;
                break;
            }

            OCCASIONALLY if (n > 0 && (n > 100000 || time(0) - saveLast > 60)) {
                // periodically note our progress, in case we are doing a lot of work and crash
                ScopedTransaction transaction(txn, MODE_X);
                Lock::GlobalWrite lk(txn->lockState());
                syncedTo = nextOpTime;
                // can't update local log ts since there are pending operations from our peer
                save(txn);
                log() << "repl:   checkpoint applied " << n << " operations" << endl;
                log() << "repl:   syncedTo: " << syncedTo.toStringLong() << endl;
                saveLast = time(0);
                n = 0;
            }

            BSONObj op = oplogReader.next();

            int b = replApplyBatchSize.get();
            bool justOne = b == 1;
            scoped_ptr<Lock::GlobalWrite> lk(justOne ? 0 : new Lock::GlobalWrite(txn->lockState()));
            while (1) {
                BSONElement ts = op.getField("ts");
                if (!(ts.type() == Date || ts.type() == Timestamp)) {
                    log() << "sync error: problem querying remote oplog record" << endl;
                    log() << "op: " << op.toString() << endl;
                    log() << "halting replication" << endl;
                    replInfo = replAllDead = "sync error: no ts found querying remote oplog record";
                    throw SyncException();
                }
                OpTime last = nextOpTime;
                nextOpTime = OpTime(ts.date());
                if (!(last < nextOpTime)) {
                    log() << "sync error: last applied optime at slave >= nextOpTime from master"
                          << endl;
                    log() << " last:       " << last.toStringLong() << endl;
                    log() << " nextOpTime: " << nextOpTime.toStringLong() << endl;
                    log() << " halting replication" << endl;
                    replInfo = replAllDead = "sync error last >= nextOpTime";
                    uassert(
                        10123,
                        "replication error last applied optime at slave >= nextOpTime from master",
                        false);
                }
                const ReplSettings& replSettings = getGlobalReplicationCoordinator()->getSettings();
                if (replSettings.slavedelay &&
                    (unsigned(time(0)) < nextOpTime.getSecs() + replSettings.slavedelay)) {
                    verify(justOne);
                    oplogReader.putBack(op);
                    _sleepAdviceTime = nextOpTime.getSecs() + replSettings.slavedelay + 1;
                    ScopedTransaction transaction(txn, MODE_X);
                    Lock::GlobalWrite lk(txn->lockState());
                    if (n > 0) {
                        syncedTo = last;
                        save(txn);
                    }
                    log() << "repl:   applied " << n << " operations" << endl;
                    log() << "repl:   syncedTo: " << syncedTo.toStringLong() << endl;
                    log() << "waiting until: " << _sleepAdviceTime << " to continue" << endl;
                    return okResultCode;
                }

                _sync_pullOpLog_applyOperation(txn, op, !justOne);
                n++;

                if (--b == 0)
                    break;
                // if to here, we are doing mulpile applications in a singel write lock acquisition
                if (!oplogReader.moreInCurrentBatch()) {
                    // break if no more in batch so we release lock while reading from the master
                    break;
                }
                op = oplogReader.next();
            }
        }
    }

    return okResultCode;
}


/* note: not yet in mutex at this point.
   returns >= 0 if ok.  return -1 if you want to reconnect.
   return value of zero indicates no sleep necessary before next call
*/
int ReplSource::sync(OperationContext* txn, int& nApplied) {
    _sleepAdviceTime = 0;
    ReplInfo r("sync");
    if (!serverGlobalParams.quiet) {
        LogstreamBuilder l = log();
        l << "repl: syncing from ";
        if (sourceName() != "main") {
            l << "source:" << sourceName() << ' ';
        }
        l << "host:" << hostName << endl;
    }
    nClonedThisPass = 0;

    // FIXME Handle cases where this db isn't on default port, or default port is spec'd in
    // hostName.
    if ((string("localhost") == hostName || string("127.0.0.1") == hostName) &&
        serverGlobalParams.port == ServerGlobalParams::DefaultDBPort) {
        log() << "repl:   can't sync from self (localhost). sources configuration may be wrong."
              << endl;
        sleepsecs(5);
        return -1;
    }

    if (!_connect(
            &oplogReader, HostAndPort(hostName), getGlobalReplicationCoordinator()->getMyRID())) {
        LOG(4) << "repl:  can't connect to sync source" << endl;
        return -1;
    }

    return _sync_pullOpLog(txn, nApplied);
}

/* --------------------------------------------------------------*/

static bool _replMainStarted = false;

/*
TODO:
_ source has autoptr to the cursor
_ reuse that cursor when we can
*/

/* returns: # of seconds to sleep before next pass
            0 = no sleep recommended
            1 = special sentinel indicating adaptive sleep recommended
*/
int _replMain(OperationContext* txn, ReplSource::SourceVector& sources, int& nApplied) {
    {
        ReplInfo r("replMain load sources");
        ScopedTransaction transaction(txn, MODE_X);
        Lock::GlobalWrite lk(txn->lockState());
        ReplSource::loadAll(txn, sources);

        // only need this param for initial reset
        _replMainStarted = true;
    }

    if (sources.empty()) {
        /* replication is not configured yet (for --slave) in local.sources.  Poll for config it
        every 20 seconds.
        */
        log() << "no source given, add a master to local.sources to start replication" << endl;
        return 20;
    }

    int sleepAdvice = 1;
    for (ReplSource::SourceVector::iterator i = sources.begin(); i != sources.end(); i++) {
        ReplSource* s = i->get();
        int res = -1;
        try {
            res = s->sync(txn, nApplied);
            bool moreToSync = s->haveMoreDbsToSync();
            if (res < 0) {
                sleepAdvice = 3;
            } else if (moreToSync) {
                sleepAdvice = 0;
            } else if (s->sleepAdvice()) {
                sleepAdvice = s->sleepAdvice();
            } else
                sleepAdvice = res;
        } catch (const SyncException&) {
            log() << "caught SyncException" << endl;
            return 10;
        } catch (AssertionException& e) {
            if (e.severe()) {
                log() << "replMain AssertionException " << e.what() << endl;
                return 60;
            } else {
                log() << "repl: AssertionException " << e.what() << endl;
            }
            replInfo = "replMain caught AssertionException";
        } catch (const DBException& e) {
            log() << "repl: DBException " << e.what() << endl;
            replInfo = "replMain caught DBException";
        } catch (const std::exception& e) {
            log() << "repl: std::exception " << e.what() << endl;
            replInfo = "replMain caught std::exception";
        } catch (...) {
            log() << "unexpected exception during replication.  replication will halt" << endl;
            replAllDead = "caught unexpected exception during replication";
        }
        if (res < 0)
            s->oplogReader.resetConnection();
    }
    return sleepAdvice;
}

static void replMain(OperationContext* txn) {
    ReplSource::SourceVector sources;
    while (1) {
        int s = 0;
        {
            ScopedTransaction transaction(txn, MODE_X);
            Lock::GlobalWrite lk(txn->lockState());
            if (replAllDead) {
                // throttledForceResyncDead can throw
                if (!getGlobalReplicationCoordinator()->getSettings().autoresync ||
                    !ReplSource::throttledForceResyncDead(txn, "auto")) {
                    log() << "all sources dead: " << replAllDead << ", sleeping for 5 seconds"
                          << endl;
                    break;
                }
            }
            // i.e., there is only one sync thread running. we will want to change/fix this.
            verify(syncing == 0);
            syncing++;
        }

        try {
            int nApplied = 0;
            s = _replMain(txn, sources, nApplied);
            if (s == 1) {
                if (nApplied == 0)
                    s = 2;
                else if (nApplied > 100) {
                    // sleep very little - just enough that we aren't truly hammering master
                    sleepmillis(75);
                    s = 0;
                }
            }
        } catch (...) {
            log() << "caught exception in _replMain" << endl;
            s = 4;
        }

        {
            ScopedTransaction transaction(txn, MODE_X);
            Lock::GlobalWrite lk(txn->lockState());
            verify(syncing == 1);
            syncing--;
        }

        if (relinquishSyncingSome) {
            relinquishSyncingSome = 0;
            s = 1;  // sleep before going back in to syncing=1
        }

        if (s) {
            stringstream ss;
            ss << "repl: sleep " << s << " sec before next pass";
            string msg = ss.str();
            if (!serverGlobalParams.quiet)
                log() << msg << endl;
            ReplInfo r(msg.c_str());
            sleepsecs(s);
        }
    }
}

static void replMasterThread() {
    sleepsecs(4);
    Client::initThread("replmaster");
    int toSleep = 10;
    while (1) {
        sleepsecs(toSleep);

        // Write a keep-alive like entry to the log. This will make things like
        // printReplicationStatus() and printSlaveReplicationStatus() stay up-to-date even
        // when things are idle.
        OperationContextImpl txn;
        txn.getClient()->getAuthorizationSession()->grantInternalAuthorization();

        Lock::GlobalWrite globalWrite(txn.lockState(), 1);
        if (globalWrite.isLocked()) {
            toSleep = 10;

            try {
                WriteUnitOfWork wuow(&txn);
                logKeepalive(&txn);
                wuow.commit();
            } catch (...) {
                log() << "caught exception in replMasterThread()" << endl;
            }
        } else {
            LOG(5) << "couldn't logKeepalive" << endl;
            toSleep = 1;
        }
    }
}

static void replSlaveThread() {
    sleepsecs(1);
    Client::initThread("replslave");

    OperationContextImpl txn;
    txn.getClient()->getAuthorizationSession()->grantInternalAuthorization();

    while (1) {
        try {
            replMain(&txn);
            sleepsecs(5);
        } catch (AssertionException&) {
            ReplInfo r("Assertion in replSlaveThread(): sleeping 5 minutes before retry");
            log() << "Assertion in replSlaveThread(): sleeping 5 minutes before retry" << endl;
            sleepsecs(300);
        } catch (DBException& e) {
            log() << "exception in replSlaveThread(): " << e.what()
                  << ", sleeping 5 minutes before retry" << endl;
            sleepsecs(300);
        } catch (...) {
            log() << "error in replSlaveThread(): sleeping 5 minutes before retry" << endl;
            sleepsecs(300);
        }
    }
}

void startMasterSlave(OperationContext* txn) {
    oldRepl();

    const ReplSettings& replSettings = getGlobalReplicationCoordinator()->getSettings();
    if (!replSettings.slave && !replSettings.master)
        return;

    txn->getClient()->getAuthorizationSession()->grantInternalAuthorization();

    {
        ReplSource temp(txn);  // Ensures local.me is populated
    }

    if (replSettings.slave) {
        verify(replSettings.slave == SimpleSlave);
        LOG(1) << "slave=true" << endl;
        boost::thread repl_thread(replSlaveThread);
    }

    if (replSettings.master) {
        LOG(1) << "master=true" << endl;
        createOplog(txn);
        boost::thread t(replMasterThread);
    }

    if (replSettings.fastsync) {
        while (!_replMainStarted)  // don't allow writes until we've set up from log
            sleepmillis(50);
    }
}
int _dummy_z;

void pretouchN(vector<BSONObj>& v, unsigned a, unsigned b) {
    Client* c = currentClient.get();
    if (c == 0) {
        Client::initThread("pretouchN");
        c = &cc();
    }

    OperationContextImpl txn;  // XXX
    ScopedTransaction transaction(&txn, MODE_S);
    Lock::GlobalRead lk(txn.lockState());

    for (unsigned i = a; i <= b; i++) {
        const BSONObj& op = v[i];
        const char* which = "o";
        const char* opType = op.getStringField("op");
        if (*opType == 'i')
            ;
        else if (*opType == 'u')
            which = "o2";
        else
            continue;
        /* todo : other operations */

        try {
            BSONObj o = op.getObjectField(which);
            BSONElement _id;
            if (o.getObjectID(_id)) {
                const char* ns = op.getStringField("ns");
                BSONObjBuilder b;
                b.append(_id);
                BSONObj result;
                Client::Context ctx(&txn, ns);
                if (Helpers::findById(&txn, ctx.db(), ns, b.done(), result))
                    _dummy_z += result.objsize();  // touch
            }
        } catch (DBException& e) {
            log() << "ignoring assertion in pretouchN() " << a << ' ' << b << ' ' << i << ' '
                  << e.toString() << endl;
        }
    }
}

void pretouchOperation(OperationContext* txn, const BSONObj& op) {
    if (txn->lockState()->isWriteLocked()) {
        // no point pretouching if write locked. not sure if this will ever fire, but just in case.
        return;
    }

    const char* which = "o";
    const char* opType = op.getStringField("op");
    if (*opType == 'i')
        ;
    else if (*opType == 'u')
        which = "o2";
    else
        return;
    /* todo : other operations */

    try {
        BSONObj o = op.getObjectField(which);
        BSONElement _id;
        if (o.getObjectID(_id)) {
            const char* ns = op.getStringField("ns");
            BSONObjBuilder b;
            b.append(_id);
            BSONObj result;
            AutoGetCollectionForRead ctx(txn, ns);
            if (Helpers::findById(txn, ctx.getDb(), ns, b.done(), result)) {
                _dummy_z += result.objsize();  // touch
            }
        }
    } catch (DBException&) {
        log() << "ignoring assertion in pretouchOperation()" << endl;
    }
}

}  // namespace repl
}  // namespace mongo
