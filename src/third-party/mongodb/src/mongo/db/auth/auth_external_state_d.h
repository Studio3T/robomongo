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
*/

#pragma once

#include "mongo/base/disallow_copying.h"
#include "mongo/base/status.h"
#include "mongo/db/auth/auth_external_state_server_common.h"

namespace mongo {

    /**
     * The implementation of AuthExternalState functionality for mongod.
     */
    class AuthExternalStateMongod : public AuthExternalStateServerCommon {
        MONGO_DISALLOW_COPYING(AuthExternalStateMongod);

    public:
        AuthExternalStateMongod();
        virtual ~AuthExternalStateMongod();

        virtual bool shouldIgnoreAuthChecks() const;

        virtual void startRequest();

    protected:
        virtual bool _findUser(const string& usersNamespace,
                               const BSONObj& query,
                               BSONObj* result) const;
    };

} // namespace mongo
