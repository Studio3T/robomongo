/**
*    Copyright (C) 2012 10gen Inc.
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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kAccessControl

#include "mongo/platform/basic.h"

#include "mongo/db/auth/authz_session_external_state_server_common.h"

#include "mongo/base/status.h"
#include "mongo/db/auth/authorization_manager.h"
#include "mongo/db/client.h"
#include "mongo/db/server_parameters.h"
#include "mongo/util/debug_util.h"
#include "mongo/util/log.h"

namespace mongo {

namespace {
MONGO_EXPORT_STARTUP_SERVER_PARAMETER(enableLocalhostAuthBypass, bool, true);
}  // namespace

// NOTE: we default _allowLocalhost to true under the assumption that _checkShouldAllowLocalhost
// will always be called before any calls to shouldAllowLocalhost.  If this is not the case,
// it could cause a security hole.
AuthzSessionExternalStateServerCommon::AuthzSessionExternalStateServerCommon(
    AuthorizationManager* authzManager)
    : AuthzSessionExternalState(authzManager), _allowLocalhost(enableLocalhostAuthBypass) {}
AuthzSessionExternalStateServerCommon::~AuthzSessionExternalStateServerCommon() {}

void AuthzSessionExternalStateServerCommon::_checkShouldAllowLocalhost(OperationContext* txn) {
    if (!_authzManager->isAuthEnabled())
        return;
    // If we know that an admin user exists, don't re-check.
    if (!_allowLocalhost)
        return;
    // Don't bother checking if we're not on a localhost connection
    if (!ClientBasic::getCurrent()->getIsLocalHostConnection()) {
        _allowLocalhost = false;
        return;
    }

    _allowLocalhost = !_authzManager->hasAnyPrivilegeDocuments(txn);
    if (_allowLocalhost) {
        ONCE {
            log() << "note: no users configured in admin.system.users, allowing localhost "
                     "access" << std::endl;
        }
    }
}

bool AuthzSessionExternalStateServerCommon::serverIsArbiter() const {
    return false;
}

bool AuthzSessionExternalStateServerCommon::shouldAllowLocalhost() const {
    ClientBasic* client = ClientBasic::getCurrent();
    return _allowLocalhost && client->getIsLocalHostConnection();
}

bool AuthzSessionExternalStateServerCommon::shouldIgnoreAuthChecks() const {
    return !_authzManager->isAuthEnabled();
}

}  // namespace mongo
