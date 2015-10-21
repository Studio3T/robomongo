// test_commands.cpp

/**
*    Copyright (C) 2013-2014 MongoDB Inc.
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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kCommand

#include "mongo/platform/basic.h"

#include "mongo/base/init.h"
#include "mongo/base/initializer_context.h"
#include "mongo/db/background.h"
#include "mongo/db/client.h"
#include "mongo/db/commands.h"
#include "mongo/db/index_builder.h"
#include "mongo/db/query/internal_plans.h"
#include "mongo/db/catalog/collection.h"
#include "mongo/db/repl/oplog.h"
#include "mongo/db/repl/replication_coordinator_global.h"
#include "mongo/db/operation_context_impl.h"
#include "mongo/util/log.h"

namespace mongo {

using std::endl;
using std::string;
using std::stringstream;

/* For testing only, not for general use. Enabled via command-line */
class GodInsert : public Command {
public:
    GodInsert() : Command("godinsert") {}
    virtual bool adminOnly() const {
        return false;
    }
    virtual bool slaveOk() const {
        return true;
    }
    virtual bool isWriteCommandForConfigServer() const {
        return false;
    }
    // No auth needed because it only works when enabled via command line.
    virtual void addRequiredPrivileges(const std::string& dbname,
                                       const BSONObj& cmdObj,
                                       std::vector<Privilege>* out) {}
    virtual void help(stringstream& help) const {
        help << "internal. for testing only.";
    }
    virtual bool run(OperationContext* txn,
                     const string& dbname,
                     BSONObj& cmdObj,
                     int,
                     string& errmsg,
                     BSONObjBuilder& result,
                     bool) {
        string coll = cmdObj["godinsert"].valuestrsafe();
        log() << "test only command godinsert invoked coll:" << coll << endl;
        uassert(13049, "godinsert must specify a collection", !coll.empty());
        string ns = dbname + "." + coll;
        BSONObj obj = cmdObj["obj"].embeddedObjectUserCheck();

        ScopedTransaction transaction(txn, MODE_IX);
        Lock::DBLock lk(txn->lockState(), dbname, MODE_X);
        WriteUnitOfWork wunit(txn);
        Client::Context ctx(txn, ns);
        Database* db = ctx.db();
        Collection* collection = db->getCollection(ns);
        if (!collection) {
            collection = db->createCollection(txn, ns);
            if (!collection) {
                errmsg = "could not create collection";
                return false;
            }
        }
        StatusWith<RecordId> res = collection->insertDocument(txn, obj, false);
        Status status = res.getStatus();
        if (status.isOK()) {
            wunit.commit();
        }
        return appendCommandStatus(result, res.getStatus());
    }
};

/* for diagnostic / testing purposes. Enabled via command line. */
class CmdSleep : public Command {
public:
    virtual bool isWriteCommandForConfigServer() const {
        return false;
    }
    virtual bool adminOnly() const {
        return true;
    }
    virtual bool slaveOk() const {
        return true;
    }
    virtual void help(stringstream& help) const {
        help << "internal testing command.  Makes db block (in a read lock) for 100 seconds\n";
        help << "w:true write lock. secs:<seconds>";
    }
    // No auth needed because it only works when enabled via command line.
    virtual void addRequiredPrivileges(const std::string& dbname,
                                       const BSONObj& cmdObj,
                                       std::vector<Privilege>* out) {}
    CmdSleep() : Command("sleep") {}
    bool run(OperationContext* txn,
             const string& ns,
             BSONObj& cmdObj,
             int,
             string& errmsg,
             BSONObjBuilder& result,
             bool fromRepl) {
        log() << "test only command sleep invoked" << endl;
        long long millis = 10 * 1000;

        if (cmdObj["secs"].isNumber() && cmdObj["millis"].isNumber()) {
            millis = cmdObj["secs"].numberLong() * 1000 + cmdObj["millis"].numberLong();
        } else if (cmdObj["secs"].isNumber()) {
            millis = cmdObj["secs"].numberLong() * 1000;
        } else if (cmdObj["millis"].isNumber()) {
            millis = cmdObj["millis"].numberLong();
        }

        if (cmdObj.getBoolField("w")) {
            ScopedTransaction transaction(txn, MODE_X);
            Lock::GlobalWrite lk(txn->lockState());
            sleepmillis(millis);
        } else {
            ScopedTransaction transaction(txn, MODE_S);
            Lock::GlobalRead lk(txn->lockState());
            sleepmillis(millis);
        }

        // Interrupt point for testing (e.g. maxTimeMS).
        txn->checkForInterrupt();

        return true;
    }
};

// Testing only, enabled via command-line.
class CapTrunc : public Command {
public:
    CapTrunc() : Command("captrunc") {}
    virtual bool slaveOk() const {
        return false;
    }
    virtual bool isWriteCommandForConfigServer() const {
        return false;
    }
    // No auth needed because it only works when enabled via command line.
    virtual void addRequiredPrivileges(const std::string& dbname,
                                       const BSONObj& cmdObj,
                                       std::vector<Privilege>* out) {}
    virtual bool run(OperationContext* txn,
                     const string& dbname,
                     BSONObj& cmdObj,
                     int,
                     string& errmsg,
                     BSONObjBuilder& result,
                     bool) {
        string coll = cmdObj["captrunc"].valuestrsafe();
        uassert(13416, "captrunc must specify a collection", !coll.empty());
        NamespaceString nss(dbname, coll);
        int n = cmdObj.getIntField("n");
        bool inc = cmdObj.getBoolField("inc");  // inclusive range?

        Client::WriteContext ctx(txn, nss.ns());
        Collection* collection = ctx.getCollection();
        massert(13417, "captrunc collection not found or empty", collection);

        RecordId end;
        {
            boost::scoped_ptr<PlanExecutor> exec(InternalPlanner::collectionScan(
                txn, nss.ns(), collection, InternalPlanner::BACKWARD));
            // We remove 'n' elements so the start is one past that
            for (int i = 0; i < n + 1; ++i) {
                PlanExecutor::ExecState state = exec->getNext(NULL, &end);
                massert(13418, "captrunc invalid n", PlanExecutor::ADVANCED == state);
            }
        }
        WriteUnitOfWork wuow(txn);
        collection->temp_cappedTruncateAfter(txn, end, inc);
        wuow.commit();
        return true;
    }
};

// Testing-only, enabled via command line.
class EmptyCapped : public Command {
public:
    EmptyCapped() : Command("emptycapped") {}
    virtual bool slaveOk() const {
        return false;
    }
    virtual bool isWriteCommandForConfigServer() const {
        return false;
    }
    virtual bool logTheOp() {
        return true;
    }
    // No auth needed because it only works when enabled via command line.
    virtual void addRequiredPrivileges(const std::string& dbname,
                                       const BSONObj& cmdObj,
                                       std::vector<Privilege>* out) {}

    virtual bool run(OperationContext* txn,
                     const string& dbname,
                     BSONObj& cmdObj,
                     int,
                     string& errmsg,
                     BSONObjBuilder& result,
                     bool fromRepl) {
        const std::string ns = parseNsCollectionRequired(dbname, cmdObj);

        ScopedTransaction scopedXact(txn, MODE_IX);
        AutoGetDb autoDb(txn, dbname, MODE_X);

        if (!fromRepl &&
            !repl::getGlobalReplicationCoordinator()->canAcceptWritesForDatabase(dbname)) {
            return appendCommandStatus(
                result,
                Status(ErrorCodes::NotMaster,
                       str::stream() << "Not primary while truncating collection " << ns));
        }

        Database* db = autoDb.getDb();
        massert(13429, "no such database", db);

        Collection* collection = db->getCollection(ns);
        massert(28584, "no such collection", collection);

        BackgroundOperation::assertNoBgOpInProgForNs(ns);

        WriteUnitOfWork wuow(txn);

        Status status = collection->truncate(txn);
        if (!status.isOK()) {
            return appendCommandStatus(result, status);
        }

        if (!fromRepl) {
            repl::logOp(txn, "c", (dbname + ".$cmd").c_str(), cmdObj);
        }

        wuow.commit();

        return true;
    }
};

// ----------------------------

MONGO_INITIALIZER(RegisterEmptyCappedCmd)(InitializerContext* context) {
    if (Command::testCommandsEnabled) {
        // Leaked intentionally: a Command registers itself when constructed.
        new CapTrunc();
        new CmdSleep();
        new EmptyCapped();
        new GodInsert();
    }
    return Status::OK();
}
}
