// AUTO-GENERATED FILE DO NOT EDIT
// See src/mongo/db/auth/generate_action_types.py
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

#include <iosfwd>
#include <map>
#include <string>

#include "mongo/base/status.h"
#include "mongo/platform/cstdint.h"

namespace mongo {

    struct ActionType {
    public:

        explicit ActionType(uint32_t identifier) : _identifier(identifier) {};
        ActionType() {};

        uint32_t getIdentifier() const {
            return _identifier;
        }

        bool operator==(const ActionType& rhs) const;

        std::string toString() const;

        // Takes the string representation of a single action type and returns the corresponding
        // ActionType enum.
        static Status parseActionFromString(const std::string& actionString, ActionType* result);

        // Takes an ActionType and returns the string representation
        static std::string actionToString(const ActionType& action);

        static const ActionType addShard;
        static const ActionType applyOps;
        static const ActionType captrunc;
        static const ActionType clean;
        static const ActionType clone;
        static const ActionType cloneCollectionLocalSource;
        static const ActionType cloneCollectionTarget;
        static const ActionType closeAllDatabases;
        static const ActionType collMod;
        static const ActionType collStats;
        static const ActionType compact;
        static const ActionType connPoolStats;
        static const ActionType connPoolSync;
        static const ActionType convertToCapped;
        static const ActionType copyDBTarget;
        static const ActionType cpuProfiler;
        static const ActionType createCollection;
        static const ActionType cursorInfo;
        static const ActionType dbHash;
        static const ActionType dbStats;
        static const ActionType diagLogging;
        static const ActionType dropCollection;
        static const ActionType dropDatabase;
        static const ActionType dropIndexes;
        static const ActionType emptycapped;
        static const ActionType enableSharding;
        static const ActionType ensureIndex;
        static const ActionType find;
        static const ActionType flushRouterConfig;
        static const ActionType fsync;
        static const ActionType getCmdLineOpts;
        static const ActionType getLog;
        static const ActionType getParameter;
        static const ActionType getShardMap;
        static const ActionType getShardVersion;
        static const ActionType handshake;
        static const ActionType hostInfo;
        static const ActionType indexStats;
        static const ActionType inprog;
        static const ActionType insert;
        static const ActionType killCursors;
        static const ActionType killop;
        static const ActionType listDatabases;
        static const ActionType listShards;
        static const ActionType logRotate;
        static const ActionType mapReduceShardedFinish;
        static const ActionType moveChunk;
        static const ActionType movePrimary;
        static const ActionType netstat;
        static const ActionType profileEnable;
        static const ActionType profileRead;
        static const ActionType reIndex;
        static const ActionType remove;
        static const ActionType removeShard;
        static const ActionType renameCollectionSameDB;
        static const ActionType repairDatabase;
        static const ActionType replSetElect;
        static const ActionType replSetFreeze;
        static const ActionType replSetFresh;
        static const ActionType replSetGetRBID;
        static const ActionType replSetGetStatus;
        static const ActionType replSetHeartbeat;
        static const ActionType replSetInitiate;
        static const ActionType replSetMaintenance;
        static const ActionType replSetReconfig;
        static const ActionType replSetStepDown;
        static const ActionType replSetSyncFrom;
        static const ActionType resync;
        static const ActionType serverStatus;
        static const ActionType setParameter;
        static const ActionType setShardVersion;
        static const ActionType shardCollection;
        static const ActionType shardingState;
        static const ActionType shutdown;
        static const ActionType split;
        static const ActionType splitChunk;
        static const ActionType splitVector;
        static const ActionType storageDetails;
        static const ActionType top;
        static const ActionType touch;
        static const ActionType unlock;
        static const ActionType unsetSharding;
        static const ActionType update;
        static const ActionType userAdmin;
        static const ActionType validate;
        static const ActionType writebacklisten;
        static const ActionType writeBacksQueued;
        static const ActionType _migrateClone;
        static const ActionType _recvChunkAbort;
        static const ActionType _recvChunkCommit;
        static const ActionType _recvChunkStart;
        static const ActionType _recvChunkStatus;
        static const ActionType _transferMods;

        enum ActionTypeIdentifier {
            addShardValue,
            applyOpsValue,
            captruncValue,
            cleanValue,
            cloneValue,
            cloneCollectionLocalSourceValue,
            cloneCollectionTargetValue,
            closeAllDatabasesValue,
            collModValue,
            collStatsValue,
            compactValue,
            connPoolStatsValue,
            connPoolSyncValue,
            convertToCappedValue,
            copyDBTargetValue,
            cpuProfilerValue,
            createCollectionValue,
            cursorInfoValue,
            dbHashValue,
            dbStatsValue,
            diagLoggingValue,
            dropCollectionValue,
            dropDatabaseValue,
            dropIndexesValue,
            emptycappedValue,
            enableShardingValue,
            ensureIndexValue,
            findValue,
            flushRouterConfigValue,
            fsyncValue,
            getCmdLineOptsValue,
            getLogValue,
            getParameterValue,
            getShardMapValue,
            getShardVersionValue,
            handshakeValue,
            hostInfoValue,
            indexStatsValue,
            inprogValue,
            insertValue,
            killCursorsValue,
            killopValue,
            listDatabasesValue,
            listShardsValue,
            logRotateValue,
            mapReduceShardedFinishValue,
            moveChunkValue,
            movePrimaryValue,
            netstatValue,
            profileEnableValue,
            profileReadValue,
            reIndexValue,
            removeValue,
            removeShardValue,
            renameCollectionSameDBValue,
            repairDatabaseValue,
            replSetElectValue,
            replSetFreezeValue,
            replSetFreshValue,
            replSetGetRBIDValue,
            replSetGetStatusValue,
            replSetHeartbeatValue,
            replSetInitiateValue,
            replSetMaintenanceValue,
            replSetReconfigValue,
            replSetStepDownValue,
            replSetSyncFromValue,
            resyncValue,
            serverStatusValue,
            setParameterValue,
            setShardVersionValue,
            shardCollectionValue,
            shardingStateValue,
            shutdownValue,
            splitValue,
            splitChunkValue,
            splitVectorValue,
            storageDetailsValue,
            topValue,
            touchValue,
            unlockValue,
            unsetShardingValue,
            updateValue,
            userAdminValue,
            validateValue,
            writebacklistenValue,
            writeBacksQueuedValue,
            _migrateCloneValue,
            _recvChunkAbortValue,
            _recvChunkCommitValue,
            _recvChunkStartValue,
            _recvChunkStatusValue,
            _transferModsValue,

            actionTypeEndValue, // Should always be last in this enum
        };

        static const int NUM_ACTION_TYPES = actionTypeEndValue;

    private:

        uint32_t _identifier; // unique identifier for this action.
    };

    // String stream operator for ActionType
    std::ostream& operator<<(std::ostream& os, const ActionType& at);

} // namespace mongo
