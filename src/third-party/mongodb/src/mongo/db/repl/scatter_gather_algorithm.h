/**
 *    Copyright 2014 MongoDB Inc.
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

#include <vector>

#include "mongo/db/repl/replication_executor.h"

namespace mongo {

template <typename T>
class StatusWith;

namespace repl {

/**
 * Interface for a specialization of a scatter-gather algorithm that sends
 * requests to a set of targets, and then processes responses until it has
 * seen enough.
 *
 * To use, call getRequests() to get a vector of request objects describing network operations.
 * Start performing the network operations in any order, and then, until
 * hasReceivedSufficientResponses() returns true, call processResponse for each response as it
 * arrives.  Once hasReceivedSufficientResponses() you may cancel outstanding network
 * operations, and must stop calling processResponse.  Implementations of this interface may
 * assume that processResponse() is never called after hasReceivedSufficientResponses() returns
 * true.
 */
class ScatterGatherAlgorithm {
public:
    /**
     * Returns the list of requests that should be sent.
     */
    virtual std::vector<ReplicationExecutor::RemoteCommandRequest> getRequests() const = 0;

    /**
     * Method to call once for each received response.
     */
    virtual void processResponse(const ReplicationExecutor::RemoteCommandRequest& request,
                                 const ResponseStatus& response) = 0;

    /**
     * Returns true if no more calls to processResponse are needed to consider the
     * algorithm complete.  Once this method returns true, one should no longer
     * call processResponse.
     */
    virtual bool hasReceivedSufficientResponses() const = 0;

protected:
    virtual ~ScatterGatherAlgorithm();  // Shouldn't actually be virtual.
};

}  // namespace repl
}  // namespace mongo
