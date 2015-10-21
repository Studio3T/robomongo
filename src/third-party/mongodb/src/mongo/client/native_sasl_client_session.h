/*    Copyright 2014 MongoDB Inc.
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
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

#include <boost/scoped_ptr.hpp>

#include "mongo/client/export_macros.h"
#include "mongo/client/sasl_client_session.h"

namespace mongo {

class SaslClientConversation;

/**
 * Implementation of the client side of a SASL authentication conversation using the
 * native SASL implementation.
 */
class MONGO_CLIENT_API NativeSaslClientSession : public SaslClientSession {
    MONGO_DISALLOW_COPYING(NativeSaslClientSession);

public:
    NativeSaslClientSession();
    ~NativeSaslClientSession();

    virtual Status initialize();

    virtual Status step(const StringData& inputData, std::string* outputData);

    virtual bool isDone() const {
        return _done;
    }

private:
    /// Number of successfully completed conversation steps.
    int _step;

    /// See isDone().
    bool _done;

    /// The client side of a SASL authentication conversation.
    boost::scoped_ptr<SaslClientConversation> _saslConversation;
};

}  // namespace mongo
