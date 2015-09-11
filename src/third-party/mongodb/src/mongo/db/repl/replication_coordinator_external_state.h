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

#pragma once

#include <boost/scoped_ptr.hpp>

#include "mongo/base/disallow_copying.h"
#include "mongo/bson/optime.h"
#include "mongo/util/time_support.h"

namespace mongo {

class BSONObj;
class OID;
class OperationContext;
class Status;
struct HostAndPort;
template <typename T>
class StatusWith;

namespace repl {

/**
 * This class represents the interface the ReplicationCoordinator uses to interact with the
 * rest of the system.  All functionality of the ReplicationCoordinatorImpl that would introduce
 * dependencies on large sections of the server code and thus break the unit testability of
 * ReplicationCoordinatorImpl should be moved here.
 */
class ReplicationCoordinatorExternalState {
    MONGO_DISALLOW_COPYING(ReplicationCoordinatorExternalState);

public:
    ReplicationCoordinatorExternalState();
    virtual ~ReplicationCoordinatorExternalState();

    /**
     * Starts the background sync, producer, and sync source feedback threads
     *
     * NOTE: Only starts threads if they are not already started,
     */
    virtual void startThreads() = 0;

    /**
     * Starts the Master/Slave threads and sets up logOp
     */
    virtual void startMasterSlave(OperationContext* txn) = 0;

    /**
     * Performs any necessary external state specific shutdown tasks, such as cleaning up
     * the threads it started.
     */
    virtual void shutdown() = 0;

    /**
     * Creates the oplog and writes the first entry.
     */
    virtual void initiateOplog(OperationContext* txn) = 0;

    /**
     * Simple wrapper around SyncSourceFeedback::forwardSlaveHandshake.  Signals to the
     * SyncSourceFeedback thread that it needs to wake up and send a replication handshake
     * upstream.
     */
    virtual void forwardSlaveHandshake() = 0;

    /**
     * Simple wrapper around SyncSourceFeedback::forwardSlaveProgress.  Signals to the
     * SyncSourceFeedback thread that it needs to wake up and send a replSetUpdatePosition
     * command upstream.
     */
    virtual void forwardSlaveProgress() = 0;

    /**
     * Queries the singleton document in local.me.  If it exists and our hostname has not
     * changed since we wrote, returns the RID stored in the object.  If the document does not
     * exist or our hostname doesn't match what was recorded in local.me, generates a new OID
     * to use as our RID, stores it in local.me, and returns it.
     */
    virtual OID ensureMe(OperationContext*) = 0;

    /**
     * Returns true if "host" is one of the network identities of this node.
     */
    virtual bool isSelf(const HostAndPort& host) = 0;

    /**
     * Gets the replica set config document from local storage, or returns an error.
     */
    virtual StatusWith<BSONObj> loadLocalConfigDocument(OperationContext* txn) = 0;

    /**
     * Stores the replica set config document in local storage, or returns an error.
     */
    virtual Status storeLocalConfigDocument(OperationContext* txn, const BSONObj& config) = 0;

    /**
     * Sets the global opTime to be 'newTime'.
     */
    virtual void setGlobalOpTime(const OpTime& newTime) = 0;

    /**
     * Gets the last optime of an operation performed on this host, from stable
     * storage.
     */
    virtual StatusWith<OpTime> loadLastOpTime(OperationContext* txn) = 0;

    /**
     * Returns the HostAndPort of the remote client connected to us that initiated the operation
     * represented by "txn".
     */
    virtual HostAndPort getClientHostAndPort(const OperationContext* txn) = 0;

    /**
     * Closes all connections except those marked with the keepOpen property, which should
     * just be connections used for heartbeating.
     * This is used during stepdown, and transition out of primary.
     */
    virtual void closeConnections() = 0;

    /**
     * Kills all operations that have a Client that is associated with an incoming user
     * connection.  Used during stepdown.
     */
    virtual void killAllUserOperations(OperationContext* txn) = 0;

    /**
     * Clears all cached sharding metadata on this server.  This is called after stepDown to
     * ensure that if the node becomes primary again in the future it will reload an up-to-date
     * version of the sharding data.
     */
    virtual void clearShardingState() = 0;

    /**
     * Notifies the bgsync and syncSourceFeedback threads to choose a new sync source.
     */
    virtual void signalApplierToChooseNewSyncSource() = 0;

    /**
     * Returns an OperationContext, owned by the caller, that may be used in methods of
     * the same instance that require an OperationContext.
     */
    virtual OperationContext* createOperationContext(const std::string& threadName) = 0;

    /**
     * Drops all temporary collections on all databases except "local".
     *
     * The implementation may assume that the caller has acquired the global exclusive lock
     * for "txn".
     */
    virtual void dropAllTempCollections(OperationContext* txn) = 0;
};

}  // namespace repl
}  // namespace mongo
