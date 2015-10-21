/**
*    Copyright (C) 2008 10gen Inc.
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

#include "mongo/platform/basic.h"

#include "mongo/base/init.h"
#include "mongo/base/status.h"
#include "mongo/bson/util/builder.h"
#include "mongo/client/dbclientinterface.h"
#include "mongo/db/auth/action_set.h"
#include "mongo/db/auth/resource_pattern.h"
#include "mongo/db/auth/authorization_session.h"
#include "mongo/db/catalog/collection.h"
#include "mongo/db/cloner.h"
#include "mongo/db/commands.h"
#include "mongo/db/commands/copydb.h"
#include "mongo/db/commands/rename_collection.h"
#include "mongo/db/dbhelpers.h"
#include "mongo/db/index_builder.h"
#include "mongo/db/instance.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/namespace_string.h"
#include "mongo/db/repl/oplog.h"
#include "mongo/db/operation_context_impl.h"
#include "mongo/db/storage_options.h"

namespace mongo {

using std::set;
using std::string;
using std::stringstream;

/* Usage:
   mydb.$cmd.findOne( { clone: "fromhost" } );
   Note: doesn't work with authentication enabled, except as internal operation or for
   old-style users for backwards compatibility.
*/
class CmdClone : public Command {
public:
    CmdClone() : Command("clone") {}

    virtual bool slaveOk() const {
        return false;
    }

    virtual bool isWriteCommandForConfigServer() const {
        return true;
    }

    virtual void help(stringstream& help) const {
        help << "clone this database from an instance of the db on another host\n";
        help << "{ clone : \"host13\" }";
    }

    virtual Status checkAuthForCommand(ClientBasic* client,
                                       const std::string& dbname,
                                       const BSONObj& cmdObj) {
        ActionSet actions;
        actions.addAction(ActionType::insert);
        actions.addAction(ActionType::createIndex);
        if (!client->getAuthorizationSession()->isAuthorizedForActionsOnResource(
                ResourcePattern::forDatabaseName(dbname), actions)) {
            return Status(ErrorCodes::Unauthorized, "Unauthorized");
        }
        return Status::OK();
    }

    virtual bool run(OperationContext* txn,
                     const string& dbname,
                     BSONObj& cmdObj,
                     int,
                     string& errmsg,
                     BSONObjBuilder& result,
                     bool fromRepl) {
        string from = cmdObj.getStringField("clone");
        if (from.empty())
            return false;

        CloneOptions opts;
        opts.fromDB = dbname;
        opts.logForRepl = !fromRepl;

        // See if there's any collections we should ignore
        if (cmdObj["collsToIgnore"].type() == Array) {
            BSONObjIterator it(cmdObj["collsToIgnore"].Obj());

            while (it.more()) {
                BSONElement e = it.next();
                if (e.type() == String) {
                    opts.collsToIgnore.insert(e.String());
                }
            }
        }

        set<string> clonedColls;

        ScopedTransaction transaction(txn, MODE_IX);
        Lock::DBLock dbXLock(txn->lockState(), dbname, MODE_X);

        Cloner cloner;
        bool rval = cloner.go(txn, dbname, from, opts, &clonedColls, errmsg);

        BSONArrayBuilder barr;
        barr.append(clonedColls);

        result.append("clonedColls", barr.arr());

        return rval;
    }
} cmdClone;

}  // namespace mongo
