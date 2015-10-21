// repair_cursor.cpp

/**
*    Copyright (C) 2014 MongoDB Inc.
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

#include <memory>

#include "mongo/db/auth/authorization_session.h"
#include "mongo/db/catalog/collection.h"
#include "mongo/db/client.h"
#include "mongo/db/commands.h"
#include "mongo/db/exec/multi_iterator.h"

namespace mongo {

using std::string;

class RepairCursorCmd : public Command {
public:
    RepairCursorCmd() : Command("repairCursor") {}

    virtual bool isWriteCommandForConfigServer() const {
        return false;
    }
    virtual bool slaveOk() const {
        return true;
    }

    virtual Status checkAuthForCommand(ClientBasic* client,
                                       const std::string& dbname,
                                       const BSONObj& cmdObj) {
        ActionSet actions;
        actions.addAction(ActionType::find);
        Privilege p(parseResourcePattern(dbname, cmdObj), actions);
        if (client->getAuthorizationSession()->isAuthorizedForPrivilege(p))
            return Status::OK();
        return Status(ErrorCodes::Unauthorized, "Unauthorized");
    }

    virtual bool run(OperationContext* txn,
                     const string& dbname,
                     BSONObj& cmdObj,
                     int options,
                     string& errmsg,
                     BSONObjBuilder& result,
                     bool fromRepl = false) {
        NamespaceString ns(parseNs(dbname, cmdObj));

        AutoGetCollectionForRead ctx(txn, ns.ns());

        Collection* collection = ctx.getCollection();
        if (!collection) {
            return appendCommandStatus(
                result, Status(ErrorCodes::NamespaceNotFound, "ns does not exist: " + ns.ns()));
        }

        std::auto_ptr<RecordIterator> iter(collection->getRecordStore()->getIteratorForRepair(txn));
        if (iter.get() == NULL) {
            return appendCommandStatus(
                result, Status(ErrorCodes::CommandNotSupported, "repair iterator not supported"));
        }

        std::auto_ptr<WorkingSet> ws(new WorkingSet());
        std::auto_ptr<MultiIteratorStage> stage(new MultiIteratorStage(txn, ws.get(), collection));
        stage->addIterator(iter.release());

        PlanExecutor* rawExec;
        Status execStatus = PlanExecutor::make(
            txn, ws.release(), stage.release(), collection, PlanExecutor::YIELD_AUTO, &rawExec);
        invariant(execStatus.isOK());
        std::auto_ptr<PlanExecutor> exec(rawExec);

        // 'exec' will be used in getMore(). It was automatically registered on construction
        // due to the auto yield policy, so it could yield during plan selection. We deregister
        // it now so that it can be registed with ClientCursor.
        exec->deregisterExec();
        exec->saveState();

        // ClientCursors' constructor inserts them into a global map that manages their
        // lifetimes. That is why the next line isn't leaky.
        ClientCursor* cc =
            new ClientCursor(collection->getCursorManager(), exec.release(), ns.ns());

        appendCursorResponseObject(cc->cursorid(), ns.ns(), BSONArray(), &result);

        return true;
    }
} repairCursorCmd;
}
