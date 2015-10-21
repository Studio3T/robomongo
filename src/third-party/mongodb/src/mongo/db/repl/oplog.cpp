// @file oplog.cpp

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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kReplication

#include "mongo/platform/basic.h"

#include "mongo/db/repl/oplog.h"

#include <deque>
#include <vector>

#include "mongo/db/auth/action_set.h"
#include "mongo/db/auth/action_type.h"
#include "mongo/db/auth/authorization_manager.h"
#include "mongo/db/auth/authorization_manager_global.h"
#include "mongo/db/background.h"
#include "mongo/db/auth/privilege.h"
#include "mongo/db/catalog/collection_catalog_entry.h"
#include "mongo/db/commands.h"
#include "mongo/db/commands/dbhash.h"
#include "mongo/db/concurrency/write_conflict_exception.h"
#include "mongo/db/dbdirectclient.h"
#include "mongo/db/dbhelpers.h"
#include "mongo/db/global_environment_experiment.h"
#include "mongo/db/global_optime.h"
#include "mongo/db/index_builder.h"
#include "mongo/db/namespace_string.h"
#include "mongo/db/ops/update.h"
#include "mongo/db/ops/update_lifecycle_impl.h"
#include "mongo/db/ops/delete.h"
#include "mongo/db/repl/bgsync.h"
#include "mongo/db/repl/replication_coordinator_global.h"
#include "mongo/db/stats/counters.h"
#include "mongo/db/operation_context_impl.h"
#include "mongo/db/storage_options.h"
#include "mongo/db/storage/storage_engine.h"
#include "mongo/db/catalog/collection.h"
#include "mongo/s/d_state.h"
#include "mongo/scripting/engine.h"
#include "mongo/util/elapsed_tracker.h"
#include "mongo/util/file.h"
#include "mongo/util/log.h"
#include "mongo/util/stacktrace.h"
#include "mongo/util/startup_test.h"

namespace mongo {

using std::endl;
using std::stringstream;

namespace repl {

namespace {
// cached copies of these...so don't rename them, drop them, etc.!!!
Database* localDB = NULL;
Collection* localOplogMainCollection = 0;
Collection* localOplogRSCollection = 0;

// Synchronizes the section where a new OpTime is generated and when it actually
// appears in the oplog.
mongo::mutex newOpMutex("oplogNewOp");
boost::condition newOptimeNotifier;

// so we can fail the same way
void checkOplogInsert(StatusWith<RecordId> result) {
    massert(17322,
            str::stream() << "write to oplog failed: " << result.getStatus().toString(),
            result.isOK());
}


/**
 * Allocates an optime for a new entry in the oplog, and updates the replication coordinator to
 * reflect that new optime.  Returns the new optime and the correct value of the "h" field for
 * the new oplog entry.
 *
 * NOTE: From the time this function returns to the time that the new oplog entry is written
 * to the storage system, all errors must be considered fatal.  This is because the this
 * function registers the new optime with the storage system and the replication coordinator,
 * and provides no facility to revert those registrations on rollback.
 */
std::pair<OpTime, long long> getNextOpTime(OperationContext* txn,
                                           Collection* oplog,
                                           const char* ns,
                                           ReplicationCoordinator* replCoord,
                                           const char* opstr) {
    mutex::scoped_lock lk(newOpMutex);
    OpTime ts = getNextGlobalOptime();
    newOptimeNotifier.notify_all();

    fassert(28560, oplog->getRecordStore()->oplogDiskLocRegister(txn, ts));

    long long hashNew;

    if (replCoord->getReplicationMode() == ReplicationCoordinator::modeReplSet) {
        hashNew = BackgroundSync::get()->getLastAppliedHash();

        // Check to make sure logOp() is legal at this point.
        if (*opstr == 'n') {
            // 'n' operations are always logged
            invariant(*ns == '\0');

            // 'n' operations do not advance the hash, since they are not rolled back
        } else {
            // Advance the hash
            hashNew = (hashNew * 131 + ts.asLL()) * 17 + replCoord->getMyId();

            BackgroundSync::get()->setLastAppliedHash(hashNew);
        }
    } else {
        hashNew = 0;
    }

    replCoord->setMyLastOptime(ts);
    return std::pair<OpTime, long long>(ts, hashNew);
}

/**
 * This allows us to stream the oplog entry directly into data region
 * main goal is to avoid copying the o portion
 * which can be very large
 * TODO: can have this build the entire doc
 */
class OplogDocWriter : public DocWriter {
public:
    OplogDocWriter(const BSONObj& frame, const BSONObj& oField) : _frame(frame), _oField(oField) {}

    ~OplogDocWriter() {}

    void writeDocument(char* start) const {
        char* buf = start;

        memcpy(buf, _frame.objdata(), _frame.objsize() - 1);  // don't copy final EOO

        reinterpret_cast<int*>(buf)[0] = documentSize();

        buf += (_frame.objsize() - 1);
        buf[0] = (char)Object;
        buf[1] = 'o';
        buf[2] = 0;
        memcpy(buf + 3, _oField.objdata(), _oField.objsize());
        buf += 3 + _oField.objsize();
        buf[0] = EOO;

        verify(static_cast<size_t>((buf + 1) - start) == documentSize());  // DEV?
    }

    size_t documentSize() const {
        return _frame.objsize() + _oField.objsize() + 1 /* type */ + 2 /* "o" */;
    }

private:
    BSONObj _frame;
    BSONObj _oField;
};

/* we write to local.oplog.rs:
     { ts : ..., h: ..., v: ..., op: ..., etc }
   ts: an OpTime timestamp
   h: hash
   v: version
   op:
    "i" insert
    "u" update
    "d" delete
    "c" db cmd
    "db" declares presence of a database (ns is set to the db name + '.')
    "n" no op

   bb param:
     if not null, specifies a boolean to pass along to the other side as b: param.
     used for "justOne" or "upsert" flags on 'd', 'u'

*/

void _logOpRS(OperationContext* txn,
              const char* opstr,
              const char* ns,
              const char* logNS,
              const BSONObj& obj,
              BSONObj* o2,
              bool* bb,
              bool fromMigrate) {
    if (strncmp(ns, "local.", 6) == 0) {
        return;
    }

    Lock::DBLock lk(txn->lockState(), "local", MODE_IX);
    Lock::OplogIntentWriteLock oplogLk(txn->lockState());

    DEV verify(logNS == 0);  // check this was never a master/slave master

    if (localOplogRSCollection == 0) {
        Client::Context ctx(txn, rsoplog);
        localDB = ctx.db();
        invariant(localDB);
        localOplogRSCollection = localDB->getCollection(rsoplog);
        massert(13347,
                "local.oplog.rs missing. did you drop it? if so restart server",
                localOplogRSCollection);
    }

    ReplicationCoordinator* replCoord = getGlobalReplicationCoordinator();
    if (ns[0] && !replCoord->canAcceptWritesForDatabase(nsToDatabaseSubstring(ns))) {
        severe() << "replSet error : logOp() but can't accept write to collection " << ns;
        fassertFailed(17405);
    }

    oplogLk.serializeIfNeeded();
    std::pair<OpTime, long long> slot =
        getNextOpTime(txn, localOplogRSCollection, ns, replCoord, opstr);

    /* we jump through a bunch of hoops here to avoid copying the obj buffer twice --
       instead we do a single copy to the destination position in the memory mapped file.
    */

    BSONObjBuilder b(256);
    b.appendTimestamp("ts", slot.first.asDate());
    b.append("h", slot.second);
    b.append("v", OPLOG_VERSION);
    b.append("op", opstr);
    b.append("ns", ns);
    if (fromMigrate)
        b.appendBool("fromMigrate", true);
    if (bb)
        b.appendBool("b", *bb);
    if (o2)
        b.append("o2", *o2);
    BSONObj partial = b.done();

    OplogDocWriter writer(partial, obj);
    checkOplogInsert(localOplogRSCollection->insertDocument(txn, &writer, false));

    txn->getClient()->setLastOp(slot.first);
}

void _logOpOld(OperationContext* txn,
               const char* opstr,
               const char* ns,
               const char* logNS,
               const BSONObj& obj,
               BSONObj* o2,
               bool* bb,
               bool fromMigrate) {
    if (strncmp(ns, "local.", 6) == 0) {
        return;
    }

    Lock::DBLock lk(txn->lockState(), "local", MODE_IX);

    if (logNS == 0) {
        logNS = "local.oplog.$main";
    }

    Lock::CollectionLock lk2(txn->lockState(), logNS, MODE_IX);

    if (localOplogMainCollection == 0) {
        Client::Context ctx(txn, logNS);
        localDB = ctx.db();
        invariant(localDB);
        localOplogMainCollection = localDB->getCollection(logNS);
        invariant(localOplogMainCollection);
    }

    ReplicationCoordinator* replCoord = getGlobalReplicationCoordinator();
    std::pair<OpTime, long long> slot =
        getNextOpTime(txn, localOplogMainCollection, ns, replCoord, opstr);

    /* we jump through a bunch of hoops here to avoid copying the obj buffer twice --
       instead we do a single copy to the destination position in the memory mapped file.
    */

    BSONObjBuilder b(256);
    b.appendTimestamp("ts", slot.first.asDate());
    b.append("op", opstr);
    b.append("ns", ns);
    if (fromMigrate)
        b.appendBool("fromMigrate", true);
    if (bb)
        b.appendBool("b", *bb);
    if (o2)
        b.append("o2", *o2);
    BSONObj partial = b.done();  // partial is everything except the o:... part.

    OplogDocWriter writer(partial, obj);
    checkOplogInsert(localOplogMainCollection->insertDocument(txn, &writer, false));

    txn->getClient()->setLastOp(slot.first);
}

void (*_logOp)(OperationContext* txn,
               const char* opstr,
               const char* ns,
               const char* logNS,
               const BSONObj& obj,
               BSONObj* o2,
               bool* bb,
               bool fromMigrate) = _logOpRS;
}  // namespace

void oldRepl() {
    _logOp = _logOpOld;
}

void logKeepalive(OperationContext* txn) {
    _logOp(txn, "n", "", 0, BSONObj(), 0, 0, false);
}
void logOpComment(OperationContext* txn, const BSONObj& obj) {
    _logOp(txn, "n", "", 0, obj, 0, 0, false);
}
void logOpInitiate(OperationContext* txn, const BSONObj& obj) {
    _logOpRS(txn, "n", "", 0, obj, 0, 0, false);
}

/*@ @param opstr:
      c userCreateNS
      i insert
      n no-op / keepalive
      d delete / remove
      u update
*/
void logOp(OperationContext* txn,
           const char* opstr,
           const char* ns,
           const BSONObj& obj,
           BSONObj* patt,
           bool* b,
           bool fromMigrate) {
    if (getGlobalReplicationCoordinator()->isReplEnabled()) {
        _logOp(txn, opstr, ns, 0, obj, patt, b, fromMigrate);
    }
    ensureShardVersionOKOrThrow(ns);

    //
    // rollback-safe logOp listeners
    //
    getGlobalAuthorizationManager()->logOp(txn, opstr, ns, obj, patt, b);
    logOpForSharding(txn, opstr, ns, obj, patt, fromMigrate);
    logOpForDbHash(txn, ns);
    if (strstr(ns, ".system.js")) {
        Scope::storedFuncMod(txn);
    }
}

OpTime writeOpsToOplog(OperationContext* txn, const std::deque<BSONObj>& ops) {
    ReplicationCoordinator* replCoord = getGlobalReplicationCoordinator();
    OpTime lastOptime;
    MONGO_WRITE_CONFLICT_RETRY_LOOP_BEGIN {
        lastOptime = replCoord->getMyLastOptime();
        invariant(!ops.empty());
        ScopedTransaction transaction(txn, MODE_IX);
        Lock::DBLock lk(txn->lockState(), "local", MODE_X);

        if (localOplogRSCollection == 0) {
            Client::Context ctx(txn, rsoplog);

            localDB = ctx.db();
            verify(localDB);
            localOplogRSCollection = localDB->getCollection(rsoplog);
            massert(13389,
                    "local.oplog.rs missing. did you drop it? if so restart server",
                    localOplogRSCollection);
        }

        Client::Context ctx(txn, rsoplog, localDB);
        WriteUnitOfWork wunit(txn);

        for (std::deque<BSONObj>::const_iterator it = ops.begin(); it != ops.end(); ++it) {
            const BSONObj& op = *it;
            const OpTime ts = op["ts"]._opTime();

            checkOplogInsert(localOplogRSCollection->insertDocument(txn, op, false));

            if (!(lastOptime < ts)) {
                severe() << "replication oplog stream went back in time. "
                            "previous timestamp: " << lastOptime << " newest timestamp: " << ts
                         << ". Op being applied: " << op;
                fassertFailedNoTrace(18905);
            }
            lastOptime = ts;
        }
        wunit.commit();

        BackgroundSync* bgsync = BackgroundSync::get();
        // Keep this up-to-date, in case we step up to primary.
        long long hash = ops.back()["h"].numberLong();
        bgsync->setLastAppliedHash(hash);

        ctx.getClient()->setLastOp(lastOptime);

        replCoord->setMyLastOptime(lastOptime);
        setNewOptime(lastOptime);

        return lastOptime;
    }
    MONGO_WRITE_CONFLICT_RETRY_LOOP_END(txn, "write oplog entry", rsoplog);
}

void createOplog(OperationContext* txn) {
    ScopedTransaction transaction(txn, MODE_X);
    Lock::GlobalWrite lk(txn->lockState());

    const char* ns = "local.oplog.$main";

    const ReplSettings& replSettings = getGlobalReplicationCoordinator()->getSettings();
    bool rs = !replSettings.replSet.empty();
    if (rs)
        ns = rsoplog;

    Client::Context ctx(txn, ns);
    Collection* collection = ctx.db()->getCollection(ns);

    if (collection) {
        if (replSettings.oplogSize != 0) {
            const CollectionOptions oplogOpts =
                collection->getCatalogEntry()->getCollectionOptions(txn);

            int o = (int)(oplogOpts.cappedSize / (1024 * 1024));
            int n = (int)(replSettings.oplogSize / (1024 * 1024));
            if (n != o) {
                stringstream ss;
                ss << "cmdline oplogsize (" << n << ") different than existing (" << o
                   << ") see: http://dochub.mongodb.org/core/increase-oplog";
                log() << ss.str() << endl;
                throw UserException(13257, ss.str());
            }
        }

        if (!rs)
            initOpTimeFromOplog(txn, ns);
        return;
    }

    /* create an oplog collection, if it doesn't yet exist. */
    long long sz = 0;
    if (replSettings.oplogSize != 0) {
        sz = replSettings.oplogSize;
    } else {
        /* not specified. pick a default size */
        sz = 50LL * 1024LL * 1024LL;
        if (sizeof(int*) >= 8) {
#if defined(__APPLE__)
            // typically these are desktops (dev machines), so keep it smallish
            sz = (256 - 64) * 1024 * 1024;
#else
            sz = 990LL * 1024 * 1024;
            double free = File::freeSpace(storageGlobalParams.dbpath);  //-1 if call not supported.
            long long fivePct = static_cast<long long>(free * 0.05);
            if (fivePct > sz)
                sz = fivePct;
            // we use 5% of free space up to 50GB (1TB free)
            static long long upperBound = 50LL * 1024 * 1024 * 1024;
            if (fivePct > upperBound)
                sz = upperBound;
#endif
        }
    }

    log() << "******" << endl;
    log() << "creating replication oplog of size: " << (int)(sz / (1024 * 1024)) << "MB..." << endl;

    CollectionOptions options;
    options.capped = true;
    options.cappedSize = sz;
    options.autoIndexId = CollectionOptions::NO;

    MONGO_WRITE_CONFLICT_RETRY_LOOP_BEGIN {
        WriteUnitOfWork uow(txn);
        invariant(ctx.db()->createCollection(txn, ns, options));
        if (!rs)
            logOp(txn, "n", "", BSONObj());
        uow.commit();
    }
    MONGO_WRITE_CONFLICT_RETRY_LOOP_END(txn, "createCollection", ns);

    /* sync here so we don't get any surprising lag later when we try to sync */
    StorageEngine* storageEngine = getGlobalEnvironment()->getGlobalStorageEngine();
    storageEngine->flushAllFiles(true);
    log() << "******" << endl;
}

// -------------------------------------

/** @param fromRepl false if from ApplyOpsCmd
    @return true if was and update should have happened and the document DNE.
    see replset initial sync code.
 */
bool applyOperation_inlock(OperationContext* txn,
                           Database* db,
                           const BSONObj& op,
                           bool fromRepl,
                           bool convertUpdateToUpsert) {
    LOG(3) << "applying op: " << op << endl;
    bool failedUpdate = false;

    OpCounters* opCounters = fromRepl ? &replOpCounters : &globalOpCounters;

    const char* names[] = {"o", "ns", "op", "b", "o2"};
    BSONElement fields[5];
    op.getFields(5, names, fields);
    BSONElement& fieldO = fields[0];
    BSONElement& fieldNs = fields[1];
    BSONElement& fieldOp = fields[2];
    BSONElement& fieldB = fields[3];
    BSONElement& fieldO2 = fields[4];

    BSONObj o;
    if (fieldO.isABSONObj())
        o = fieldO.embeddedObject();

    const char* ns = fieldNs.valuestrsafe();

    BSONObj o2;
    if (fieldO2.isABSONObj())
        o2 = fieldO2.Obj();

    bool valueB = fieldB.booleanSafe();

    if (nsIsFull(ns)) {
        if (supportsDocLocking()) {
            // WiredTiger, and others requires MODE_IX since the applier threads driving
            // this allow writes to the same collection on any thread.
            invariant(txn->lockState()->isCollectionLockedForMode(ns, MODE_IX));
        } else {
            // mmapV1 ensures that all operations to the same collection are executed from
            // the same worker thread, so it takes an exclusive lock (MODE_X)
            invariant(txn->lockState()->isCollectionLockedForMode(ns, MODE_X));
        }
    }
    Collection* collection = db->getCollection(ns);
    IndexCatalog* indexCatalog = collection == NULL ? NULL : collection->getIndexCatalog();

    // operation type -- see logOp() comments for types
    const char* opType = fieldOp.valuestrsafe();

    if (*opType == 'i') {
        opCounters->gotInsert();

        const char* p = strchr(ns, '.');
        if (p && nsToCollectionSubstring(p) == "system.indexes") {
            if (o["background"].trueValue()) {
                IndexBuilder* builder = new IndexBuilder(o);
                // This spawns a new thread and returns immediately.
                builder->go();
                // Wait for thread to start and register itself
                Lock::TempRelease release(txn->lockState());
                IndexBuilder::waitForBgIndexStarting();
            } else {
                IndexBuilder builder(o);
                Status status = builder.buildInForeground(txn, db);
                uassertStatusOK(status);
            }
        } else {
            // do upserts for inserts as we might get replayed more than once
            OpDebug debug;
            BSONElement _id;
            if (!o.getObjectID(_id)) {
                /* No _id.  This will be very slow. */
                Timer t;

                const NamespaceString requestNs(ns);
                UpdateRequest request(requestNs);

                request.setQuery(o);
                request.setUpdates(o);
                request.setUpsert();
                request.setFromReplication();
                UpdateLifecycleImpl updateLifecycle(true, requestNs);
                request.setLifecycle(&updateLifecycle);

                update(txn, db, request, &debug);

                if (t.millis() >= 2) {
                    RARELY OCCASIONALLY log()
                        << "warning, repl doing slow updates (no _id field) for " << ns << endl;
                }
            } else {
                /* todo : it may be better to do an insert here, and then catch the dup key
                 * exception and do update then.  very few upserts will not be inserts...
                          */
                BSONObjBuilder b;
                b.append(_id);

                const NamespaceString requestNs(ns);
                UpdateRequest request(requestNs);

                request.setQuery(b.done());
                request.setUpdates(o);
                request.setUpsert();
                request.setFromReplication();
                UpdateLifecycleImpl updateLifecycle(true, requestNs);
                request.setLifecycle(&updateLifecycle);

                update(txn, db, request, &debug);
            }
        }
    } else if (*opType == 'u') {
        opCounters->gotUpdate();

        OpDebug debug;
        BSONObj updateCriteria = o2;
        const bool upsert = valueB || convertUpdateToUpsert;

        const NamespaceString requestNs(ns);
        UpdateRequest request(requestNs);

        request.setQuery(updateCriteria);
        request.setUpdates(o);
        request.setUpsert(upsert);
        request.setFromReplication();
        UpdateLifecycleImpl updateLifecycle(true, requestNs);
        request.setLifecycle(&updateLifecycle);

        UpdateResult ur = update(txn, db, request, &debug);

        if (ur.numMatched == 0) {
            if (ur.modifiers) {
                if (updateCriteria.nFields() == 1) {
                    // was a simple { _id : ... } update criteria
                    failedUpdate = true;
                    log() << "replication failed to apply update: " << op.toString() << endl;
                }
                // need to check to see if it isn't present so we can set failedUpdate correctly.
                // note that adds some overhead for this extra check in some cases, such as an
                // updateCriteria of the form
                //   { _id:..., { x : {$size:...} }
                // thus this is not ideal.
                else {
                    if (collection == NULL ||
                        (indexCatalog->haveIdIndex(txn) &&
                         Helpers::findById(txn, collection, updateCriteria).isNull()) ||
                        // capped collections won't have an _id index
                        (!indexCatalog->haveIdIndex(txn) &&
                         Helpers::findOne(txn, collection, updateCriteria, false).isNull())) {
                        failedUpdate = true;
                        log() << "replication couldn't find doc: " << op.toString() << endl;
                    }

                    // Otherwise, it's present; zero objects were updated because of additional
                    // specifiers in the query for idempotence
                }
            } else {
                // this could happen benignly on an oplog duplicate replay of an upsert
                // (because we are idempotent),
                // if an regular non-mod update fails the item is (presumably) missing.
                if (!upsert) {
                    failedUpdate = true;
                    log() << "replication update of non-mod failed: " << op.toString() << endl;
                }
            }
        }
    } else if (*opType == 'd') {
        opCounters->gotDelete();
        if (opType[1] == 0)
            deleteObjects(txn, db, ns, o, PlanExecutor::YIELD_MANUAL, /*justOne*/ valueB);
        else
            verify(opType[1] == 'b');  // "db" advertisement
    } else if (*opType == 'c') {
        bool done = false;
        while (!done) {
            BufBuilder bb;
            BSONObjBuilder runCommandResult;

            // Applying commands in repl is done under Global W-lock, so it is safe to not
            // perform the current DB checks after reacquiring the lock.
            invariant(txn->lockState()->isW());

            _runCommands(txn, ns, o, bb, runCommandResult, true, 0);
            // _runCommands takes care of adjusting opcounters for command counting.
            Status status = Command::getStatusFromCommandResult(runCommandResult.done());
            switch (status.code()) {
                case ErrorCodes::WriteConflict: {
                    // Need to throw this up to a higher level where it will be caught and the
                    // operation retried.
                    throw WriteConflictException();
                }
                case ErrorCodes::BackgroundOperationInProgressForDatabase: {
                    Lock::TempRelease release(txn->lockState());

                    BackgroundOperation::awaitNoBgOpInProgForDb(nsToDatabaseSubstring(ns));
                    txn->recoveryUnit()->commitAndRestart();
                    break;
                }
                case ErrorCodes::BackgroundOperationInProgressForNamespace: {
                    Lock::TempRelease release(txn->lockState());

                    Command* cmd = Command::findCommand(o.firstElement().fieldName());
                    invariant(cmd);
                    BackgroundOperation::awaitNoBgOpInProgForNs(cmd->parseNs(nsToDatabase(ns), o));
                    txn->recoveryUnit()->commitAndRestart();
                    break;
                }
                default:
                    warning() << "repl Failed command " << o << " on " << nsToDatabaseSubstring(ns)
                              << " with status " << status << " during oplog application";
                // fallthrough
                case ErrorCodes::OK:
                    done = true;
                    break;
            }
        }
    } else if (*opType == 'n') {
        // no op
    } else {
        throw MsgAssertionException(14825,
                                    ErrorMsg("error in applyOperation : unknown opType ", *opType));
    }

    // AuthorizationManager's logOp method registers a RecoveryUnit::Change
    // and to do so we need to have begun a UnitOfWork
    WriteUnitOfWork wuow(txn);
    getGlobalAuthorizationManager()->logOp(
        txn, opType, ns, o, fieldO2.isABSONObj() ? &o2 : NULL, !fieldB.eoo() ? &valueB : NULL);
    wuow.commit();

    return failedUpdate;
}

void waitUpToOneSecondForOptimeChange(const OpTime& referenceTime) {
    mutex::scoped_lock lk(newOpMutex);

    while (referenceTime == getLastSetOptime()) {
        if (!newOptimeNotifier.timed_wait(lk.boost(), boost::posix_time::seconds(1)))
            return;
    }
}

void setNewOptime(const OpTime& newTime) {
    mutex::scoped_lock lk(newOpMutex);
    setGlobalOptime(newTime);
    newOptimeNotifier.notify_all();
}

void initOpTimeFromOplog(OperationContext* txn, const std::string& oplogNS) {
    DBDirectClient c(txn);
    BSONObj lastOp = c.findOne(oplogNS, Query().sort(reverseNaturalObj), NULL, QueryOption_SlaveOk);

    if (!lastOp.isEmpty()) {
        LOG(1) << "replSet setting last OpTime";
        setNewOptime(lastOp["ts"].date());
    }
}

void oplogCheckCloseDatabase(OperationContext* txn, Database* db) {
    invariant(txn->lockState()->isW());

    localDB = NULL;
    localOplogMainCollection = NULL;
    localOplogRSCollection = NULL;
}

}  // namespace repl
}  // namespace mongo
