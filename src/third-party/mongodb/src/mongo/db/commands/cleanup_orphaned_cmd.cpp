/**
 *    Copyright (C) 2013 10gen Inc.
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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kCommand

#include "mongo/platform/basic.h"

#include <string>
#include <vector>

#include "mongo/base/init.h"
#include "mongo/db/auth/action_type.h"
#include "mongo/db/auth/authorization_manager.h"
#include "mongo/db/auth/authorization_session.h"
#include "mongo/db/auth/privilege.h"
#include "mongo/db/commands.h"
#include "mongo/db/field_parser.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/namespace_string.h"
#include "mongo/db/range_deleter_service.h"
#include "mongo/db/repl/replication_coordinator_global.h"
#include "mongo/s/collection_metadata.h"
#include "mongo/s/d_state.h"
#include "mongo/s/range_arithmetic.h"
#include "mongo/s/type_settings.h"
#include "mongo/util/log.h"

namespace {
using mongo::WriteConcernOptions;

const int kDefaultWTimeoutMs = 60 * 1000;
const WriteConcernOptions DefaultWriteConcern("majority",
                                              WriteConcernOptions::NONE,
                                              kDefaultWTimeoutMs);
}

namespace mongo {

using std::endl;
using std::string;

using mongoutils::str::stream;

enum CleanupResult { CleanupResult_Done, CleanupResult_Continue, CleanupResult_Error };

/**
 * Cleans up one range of orphaned data starting from a range that overlaps or starts at
 * 'startingFromKey'.  If empty, startingFromKey is the minimum key of the sharded range.
 *
 * @return CleanupResult_Continue and 'stoppedAtKey' if orphaned range was found and cleaned
 * @return CleanupResult_Done if no orphaned ranges remain
 * @return CleanupResult_Error and 'errMsg' if an error occurred
 *
 * If the collection is not sharded, returns CleanupResult_Done.
 */
CleanupResult cleanupOrphanedData(OperationContext* txn,
                                  const NamespaceString& ns,
                                  const BSONObj& startingFromKeyConst,
                                  const WriteConcernOptions& secondaryThrottle,
                                  BSONObj* stoppedAtKey,
                                  string* errMsg) {
    BSONObj startingFromKey = startingFromKeyConst;

    CollectionMetadataPtr metadata = shardingState.getCollectionMetadata(ns.toString());
    if (!metadata || metadata->getKeyPattern().isEmpty()) {
        warning() << "skipping orphaned data cleanup for " << ns.toString()
                  << ", collection is not sharded" << endl;

        return CleanupResult_Done;
    }

    BSONObj keyPattern = metadata->getKeyPattern();
    if (!startingFromKey.isEmpty()) {
        if (!metadata->isValidKey(startingFromKey)) {
            *errMsg = stream() << "could not cleanup orphaned data, start key " << startingFromKey
                               << " does not match shard key pattern " << keyPattern;

            warning() << *errMsg << endl;
            return CleanupResult_Error;
        }
    } else {
        startingFromKey = metadata->getMinKey();
    }

    KeyRange orphanRange;
    if (!metadata->getNextOrphanRange(startingFromKey, &orphanRange)) {
        LOG(1) << "orphaned data cleanup requested for " << ns.toString() << " starting from "
               << startingFromKey << ", no orphan ranges remain" << endl;

        return CleanupResult_Done;
    }
    orphanRange.ns = ns;
    *stoppedAtKey = orphanRange.maxKey;

    // We're done with this metadata now, no matter what happens
    metadata.reset();

    LOG(1) << "orphaned data cleanup requested for " << ns.toString() << " starting from "
           << startingFromKey << ", removing next orphan range"
           << " [" << orphanRange.minKey << "," << orphanRange.maxKey << ")" << endl;

    // Metadata snapshot may be stale now, but deleter checks metadata again in write lock
    // before delete.
    RangeDeleterOptions deleterOptions(orphanRange);
    deleterOptions.writeConcern = secondaryThrottle;
    deleterOptions.onlyRemoveOrphanedDocs = true;
    deleterOptions.fromMigrate = true;
    // Must wait for cursors since there can be existing cursors with an older
    // CollectionMetadata.
    deleterOptions.waitForOpenCursors = true;
    deleterOptions.removeSaverReason = "cleanup-cmd";

    if (!getDeleter()->deleteNow(txn, deleterOptions, errMsg)) {
        warning() << *errMsg << endl;
        return CleanupResult_Error;
    }

    return CleanupResult_Continue;
}

/**
 * Cleanup orphaned data command.  Called on a particular namespace, and if the collection
 * is sharded will clean up a single orphaned data range which overlaps or starts after a
 * passed-in 'startingFromKey'.  Returns true and a 'stoppedAtKey' (which will start a
 * search for the next orphaned range if the command is called again) or no key if there
 * are no more orphaned ranges in the collection.
 *
 * If the collection is not sharded, returns true but no 'stoppedAtKey'.
 * On failure, returns false and an error message.
 *
 * Calling this command repeatedly until no 'stoppedAtKey' is returned ensures that the
 * full collection range is searched for orphaned documents, but since sharding state may
 * change between calls there is no guarantee that all orphaned documents were found unless
 * the balancer is off.
 *
 * Safe to call with the balancer on.
 *
 * Format:
 *
 * {
 *      cleanupOrphaned: <ns>,
 *      // optional parameters:
 *      startingAtKey: { <shardKeyValue> }, // defaults to lowest value
 *      secondaryThrottle: <bool>, // defaults to true
 *      // defaults to { w: "majority", wtimeout: 60000 }. Applies to individual writes.
 *      writeConcern: { <writeConcern options> }
 * }
 */
class CleanupOrphanedCommand : public Command {
public:
    CleanupOrphanedCommand() : Command("cleanupOrphaned") {}

    virtual bool slaveOk() const {
        return false;
    }
    virtual bool adminOnly() const {
        return true;
    }
    virtual bool localHostOnlyIfNoAuth(const BSONObj& cmdObj) {
        return false;
    }

    virtual Status checkAuthForCommand(ClientBasic* client,
                                       const std::string& dbname,
                                       const BSONObj& cmdObj) {
        if (!client->getAuthorizationSession()->isAuthorizedForActionsOnResource(
                ResourcePattern::forClusterResource(), ActionType::cleanupOrphaned)) {
            return Status(ErrorCodes::Unauthorized, "Not authorized for cleanupOrphaned command.");
        }
        return Status::OK();
    }

    virtual bool isWriteCommandForConfigServer() const {
        return false;
    }

    // Input
    static BSONField<string> nsField;
    static BSONField<BSONObj> startingFromKeyField;

    // Output
    static BSONField<BSONObj> stoppedAtKeyField;

    bool run(OperationContext* txn,
             string const& db,
             BSONObj& cmdObj,
             int,
             string& errmsg,
             BSONObjBuilder& result,
             bool) {
        string ns;
        if (!FieldParser::extract(cmdObj, nsField, &ns, &errmsg)) {
            return false;
        }

        if (ns == "") {
            errmsg = "no collection name specified";
            return false;
        }

        BSONObj startingFromKey;
        if (!FieldParser::extract(cmdObj, startingFromKeyField, &startingFromKey, &errmsg)) {
            return false;
        }

        WriteConcernOptions writeConcern;
        Status status = writeConcern.parseSecondaryThrottle(cmdObj, NULL);

        if (!status.isOK()) {
            if (status.code() != ErrorCodes::WriteConcernNotDefined) {
                return appendCommandStatus(result, status);
            }

            writeConcern = DefaultWriteConcern;
        } else {
            repl::ReplicationCoordinator* replCoordinator = repl::getGlobalReplicationCoordinator();
            Status status = replCoordinator->checkIfWriteConcernCanBeSatisfied(writeConcern);

            if (replCoordinator->getReplicationMode() ==
                    repl::ReplicationCoordinator::modeMasterSlave &&
                writeConcern.shouldWaitForOtherNodes()) {
                warning() << "cleanupOrphaned cannot check if write concern setting "
                          << writeConcern.toBSON()
                          << " can be enforced in a master slave configuration";
            }

            if (!status.isOK() && status != ErrorCodes::NoReplicationEnabled) {
                return appendCommandStatus(result, status);
            }
        }

        if (writeConcern.shouldWaitForOtherNodes() &&
            writeConcern.wTimeout == WriteConcernOptions::kNoTimeout) {
            // Don't allow no timeout.
            writeConcern.wTimeout = kDefaultWTimeoutMs;
        }

        if (!shardingState.enabled()) {
            errmsg = str::stream() << "server is not part of a sharded cluster or "
                                   << "the sharding metadata is not yet initialized.";
            return false;
        }

        ChunkVersion shardVersion;
        status = shardingState.refreshMetadataNow(txn, ns, &shardVersion);
        if (!status.isOK()) {
            if (status.code() == ErrorCodes::RemoteChangeDetected) {
                warning() << "Shard version in transition detected while refreshing "
                          << "metadata for " << ns << " at version " << shardVersion << endl;
            } else {
                errmsg = str::stream() << "failed to refresh shard metadata: " << status.reason();
                return false;
            }
        }

        BSONObj stoppedAtKey;
        CleanupResult cleanupResult = cleanupOrphanedData(
            txn, NamespaceString(ns), startingFromKey, writeConcern, &stoppedAtKey, &errmsg);

        if (cleanupResult == CleanupResult_Error) {
            return false;
        }

        if (cleanupResult == CleanupResult_Continue) {
            result.append(stoppedAtKeyField(), stoppedAtKey);
        } else {
            dassert(cleanupResult == CleanupResult_Done);
        }

        return true;
    }
};

BSONField<string> CleanupOrphanedCommand::nsField("cleanupOrphaned");
BSONField<BSONObj> CleanupOrphanedCommand::startingFromKeyField("startingFromKey");
BSONField<BSONObj> CleanupOrphanedCommand::stoppedAtKeyField("stoppedAtKey");

MONGO_INITIALIZER(RegisterCleanupOrphanedCommand)(InitializerContext* context) {
    // Leaked intentionally: a Command registers itself when constructed.
    new CleanupOrphanedCommand();
    return Status::OK();
}

}  // namespace mongo
