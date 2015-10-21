/*
 *    Copyright (C) 2014 10gen, Inc.
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

#pragma once

#include <boost/scoped_ptr.hpp>
#include <string>

#include "mongo/base/disallow_copying.h"
#include "mongo/base/status.h"
#include "mongo/base/string_data.h"
#include "mongo/db/auth/authentication_session.h"
#include "mongo/platform/cstdint.h"
#include "mongo/db/auth/sasl_authentication_session.h"
#include "mongo/db/auth/sasl_server_conversation.h"

namespace mongo {

/**
 * Authentication session data for the server side of SASL authentication.
 */
class NativeSaslAuthenticationSession : public SaslAuthenticationSession {
    MONGO_DISALLOW_COPYING(NativeSaslAuthenticationSession);

public:
    explicit NativeSaslAuthenticationSession(AuthorizationSession* authSession);
    virtual ~NativeSaslAuthenticationSession();

    virtual Status start(const StringData& authenticationDatabase,
                         const StringData& mechanism,
                         const StringData& serviceName,
                         const StringData& serviceHostname,
                         int64_t conversationId,
                         bool autoAuthorize);

    virtual Status step(const StringData& inputData, std::string* outputData);

    virtual std::string getPrincipalId() const;

    virtual const char* getMechanism() const;

private:
    std::string _mechanism;
    boost::scoped_ptr<SaslServerConversation> _saslConversation;
};
}  // namespace mongo
