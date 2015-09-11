/**
*    Copyright (C) 2015 MongoDB Inc.
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

#include "mongo/db/commands/rename_collection.h"

#include <string>

#include "mongo/db/auth/action_set.h"
#include "mongo/db/auth/action_type.h"
#include "mongo/db/auth/authorization_session.h"
#include "mongo/db/auth/privilege.h"
#include "mongo/db/client_basic.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/namespace_string.h"

namespace mongo {

Status checkAuthForListCollectionsCommand(ClientBasic* client,
                                          const std::string& dbname,
                                          const BSONObj& cmdObj) {
    AuthorizationSession* authzSession = client->getAuthorizationSession();

    // Check for the listCollections ActionType on the database
    // or find on system.namespaces for pre 3.0 systems.
    if (authzSession->isAuthorizedForActionsOnResource(ResourcePattern::forDatabaseName(dbname),
                                                       ActionType::listCollections)) {
        return Status::OK();
    }

    if (authzSession->isAuthorizedForActionsOnResource(
            ResourcePattern::forExactNamespace(NamespaceString(dbname, "system.namespaces")),
            ActionType::find)) {
        return Status::OK();
    }

    return Status(ErrorCodes::Unauthorized,
                  str::stream() << "Not authorized to list collections on db: " << dbname);
}

}  // namespace mongo
