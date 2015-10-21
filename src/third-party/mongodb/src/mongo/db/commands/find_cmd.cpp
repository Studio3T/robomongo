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

#include "mongo/platform/basic.h"

#include "mongo/db/commands/find_cmd.h"

#include <boost/scoped_ptr.hpp>

#include "mongo/db/auth/authorization_session.h"
#include "mongo/db/catalog/collection.h"
#include "mongo/db/catalog/database.h"
#include "mongo/db/client.h"
#include "mongo/db/commands.h"
#include "mongo/db/query/explain.h"
#include "mongo/db/query/get_executor.h"
#include "mongo/db/query/find.h"
#include "mongo/s/d_state.h"

namespace mongo {

using boost::scoped_ptr;
using std::auto_ptr;
using std::string;

static FindCmd findCmd;

Status FindCmd::checkAuthForCommand(ClientBasic* client,
                                    const std::string& dbname,
                                    const BSONObj& cmdObj) {
    AuthorizationSession* authzSession = client->getAuthorizationSession();
    ResourcePattern pattern = parseResourcePattern(dbname, cmdObj);

    if (authzSession->isAuthorizedForActionsOnResource(pattern, ActionType::find)) {
        return Status::OK();
    }

    return Status(ErrorCodes::Unauthorized, "unauthorized");
}

Status FindCmd::explain(OperationContext* txn,
                        const std::string& dbname,
                        const BSONObj& cmdObj,
                        ExplainCommon::Verbosity verbosity,
                        BSONObjBuilder* out) const {
    const string fullns = parseNs(dbname, cmdObj);

    // Parse the command BSON to a LiteParsedQuery.
    LiteParsedQuery* rawLpq;
    bool isExplain = true;
    Status lpqStatus = LiteParsedQuery::make(fullns, cmdObj, isExplain, &rawLpq);
    if (!lpqStatus.isOK()) {
        return lpqStatus;
    }
    auto_ptr<LiteParsedQuery> lpq(rawLpq);

    const NamespaceString nss(fullns);

    // Finish the parsing step by using the LiteParsedQuery to create a CanonicalQuery.
    // This requires a lock on the collection in case we're parsing $where: where-specific
    // parsing code assumes we have a lock and creates execution machinery that requires it.
    CanonicalQuery* rawCq;
    WhereCallbackReal whereCallback(txn, nss.db());
    Status canonStatus = CanonicalQuery::canonicalize(lpq.release(), &rawCq, whereCallback);
    if (!canonStatus.isOK()) {
        return canonStatus;
    }
    auto_ptr<CanonicalQuery> cq(rawCq);

    AutoGetCollectionForRead ctx(txn, nss);
    // The collection may be NULL. If so, getExecutor() should handle it by returning
    // an execution tree with an EOFStage.
    Collection* collection = ctx.getCollection();

    // We have a parsed query. Time to get the execution plan for it.
    PlanExecutor* rawExec;
    Status execStatus = Status::OK();
    if (cq->getParsed().getOptions().oplogReplay) {
        execStatus = getOplogStartHack(txn, collection, cq.release(), &rawExec);
    } else {
        size_t options = QueryPlannerParams::DEFAULT;
        // TODO: The version attached to the TLS cannot be relied upon, the shard
        // version should be passed as part of the command parameter.
        if (shardingState.needCollectionMetadata(cq->getParsed().ns())) {
            options |= QueryPlannerParams::INCLUDE_SHARD_FILTER;
        }

        execStatus =
            getExecutor(txn, collection, cq.release(), PlanExecutor::YIELD_AUTO, &rawExec, options);
    }

    if (!execStatus.isOK()) {
        return execStatus;
    }

    scoped_ptr<PlanExecutor> exec(rawExec);

    // Got the execution tree. Explain it.
    Explain::explainStages(exec.get(), verbosity, out);
    return Status::OK();
}

bool FindCmd::run(OperationContext* txn,
                  const string& dbname,
                  BSONObj& cmdObj,
                  int options,
                  string& errmsg,
                  BSONObjBuilder& result,
                  bool fromRepl) {
    // Currently only explains of finds run through the find command. Queries that are not
    // explained use the legacy OP_QUERY path.
    // TODO: check the comment above regarding shard versioning.
    errmsg = "find command not yet implemented";
    return false;
}

}  // namespace mongo
