/*    Copyright 2012 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include "mongo/base/status.h"
#include "mongo/bson/bsontypes.h"
#include "mongo/client/dbclientinterface.h"

struct Gsasl;

namespace mongo {
    class BSONObj;

    /**
     * Attempts to authenticate "client" using the SASL protocol.
     *
     * Requires an initialized instance of the "gsasl" library, as the first
     * parameter.
     *
     * The "saslParameters" BSONObj should be initialized with zero or more of the
     * fields below.  Which fields are required depends on the mechanism.  Consult the
     * libgsasl documentation.
     *
     *     "mechanism": The string name of the sasl mechanism to use.  Mandatory.
     *     "autoAuthorize": Truthy values tell the server to automatically acquire privileges on
     *         all resources after successful authentication, which is the default.  Falsey values
     *         instruct the server to await separate privilege-acquisition commands.
     *     "user": The string name of the principal to authenticate, GSASL_AUTHID.
     *     "userSource": The database target of the auth command, which identifies the location
     *         of the credential information for the principal.  May be "$sasl" if credential
     *         information is stored outside of the mongo cluster.
     *     "pwd": The password data, GSASL_PASSWORD.
     *     "serviceName": The GSSAPI service name to use.  Defaults to "mongodb".
     *     "serviceHostname": The GSSAPI hostname to use.  Defaults to the name of the remote host.
     *
     * Other fields in saslParameters are silently ignored.
     *
     * "sessionHook" is a pointer to optional data, which may be used by the gsasl_callback
     * previously set on "gsasl".  The session hook is set on an underlying Gsasl_session using
     * gsasl_session_hook_set, and may be accessed by callbacks using gsasl_session_hook_get.
     * See the gsasl documentation.
     *
     * Returns an OK status on success, and ErrorCodes::AuthenticationFailed if authentication is
     * rejected.  Other failures, all of which are tantamount to authentication failure, may also be
     * returned.
     */
    Status saslClientAuthenticate(Gsasl *gsasl,
                                  DBClientWithCommands* client,
                                  const BSONObj& saslParameters,
                                  void* sessionHook);

    /**
     * Extracts the payload field from "cmdObj", and store it into "*payload".
     *
     * Sets "*type" to the BSONType of the payload field in cmdObj.
     *
     * If the type of the payload field is String, the contents base64 decodes and
     * stores into "*payload".  If the type is BinData, the contents are stored directly
     * into "*payload".  In all other cases, returns
     */
    Status saslExtractPayload(const BSONObj& cmdObj, std::string* payload, BSONType* type);

    // Constants

    /// String name of the saslStart command.
    extern const char* const saslStartCommandName;

    /// String name of the saslContinue command.
    extern const char* const saslContinueCommandName;

    /// Name of the saslStart parameter indicating that the server should automatically grant the
    /// connection all privileges associated with the principal after successful authentication.
    extern const char* const saslCommandAutoAuthorizeFieldName;

    /// Name of the field contain the status code in responses from the server.
    extern const char* const saslCommandCodeFieldName;

    /// Name of the field containing the conversation identifier in server respones and saslContinue
    /// commands.
    extern const char* const saslCommandConversationIdFieldName;

    /// Name of the field that indicates whether or not the server believes authentication has
    /// completed successfully.
    extern const char* const saslCommandDoneFieldName;

    /// Field in which  to store error messages associated with non-success return codes.
    extern const char* const saslCommandErrmsgFieldName;

    /// Name of parameter to saslStart command indiciating the client's desired sasl mechanism.
    extern const char* const saslCommandMechanismFieldName;

    /// In the event that saslStart supplies an unsupported mechanism, the server responds with a
    /// field by this name, with a list of supported mechanisms.
    extern const char* const saslCommandMechanismListFieldName;

    /// Field containing password information for saslClientAuthenticate().
    extern const char* const saslCommandPasswordFieldName;

    /// Field containing sasl payloads passed to and from the server.
    extern const char* const saslCommandPayloadFieldName;

    /// Field containing the string identifier of the principal to authenticate in
    /// saslClientAuthenticate().
    extern const char* const saslCommandPrincipalFieldName;

    /// Field containing the string identifier of the database containing credential information,
    /// or "$sasl" if the credential information is stored outside of the mongo cluster.
    extern const char* const saslCommandPrincipalSourceFieldName;

    /// Field overriding the FQDN of the hostname hosting the mongodb srevice in
    /// saslClientAuthenticate().
    extern const char* const saslCommandServiceHostnameFieldName;

    /// Field overriding the name of the mongodb service saslClientAuthenticate().
    extern const char* const saslCommandServiceNameFieldName;

    /// Default database against which sasl authentication commands should run.
    extern const char* const saslDefaultDBName;

    /// Default sasl service name, "mongodb".
    extern const char* const saslDefaultServiceName;
}
