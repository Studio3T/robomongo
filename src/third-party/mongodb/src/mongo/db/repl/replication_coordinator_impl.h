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
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <vector>

#include "mongo/base/status.h"
#include "mongo/bson/optime.h"
#include "mongo/db/global_environment_experiment.h"
#include "mongo/db/repl/member_state.h"
#include "mongo/db/repl/replica_set_config.h"
#include "mongo/db/repl/replication_coordinator.h"
#include "mongo/db/repl/replication_coordinator_external_state.h"
#include "mongo/db/repl/replication_executor.h"
#include "mongo/db/repl/update_position_args.h"
#include "mongo/platform/atomic_word.h"
#include "mongo/platform/unordered_map.h"
#include "mongo/platform/unordered_set.h"
#include "mongo/util/net/hostandport.h"

namespace mongo {

class Timer;
template <typename T>
class StatusWith;

namespace repl {

class ElectCmdRunner;
class FreshnessChecker;
class HeartbeatResponseAction;
class OplogReader;
class SyncSourceFeedback;
class TopologyCoordinator;

class ReplicationCoordinatorImpl : public ReplicationCoordinator, public KillOpListenerInterface {
    MONGO_DISALLOW_COPYING(ReplicationCoordinatorImpl);

public:
    // Takes ownership of the "externalState", "topCoord" and "network" objects.
    ReplicationCoordinatorImpl(const ReplSettings& settings,
                               ReplicationCoordinatorExternalState* externalState,
                               ReplicationExecutor::NetworkInterface* network,
                               TopologyCoordinator* topoCoord,
                               int64_t prngSeed);
    virtual ~ReplicationCoordinatorImpl();

    // ================== Members of public ReplicationCoordinator API ===================

    virtual void startReplication(OperationContext* txn);

    virtual void shutdown();

    virtual const ReplSettings& getSettings() const;

    virtual Mode getReplicationMode() const;

    virtual MemberState getMemberState() const;

    virtual bool isInPrimaryOrSecondaryState() const;

    virtual Seconds getSlaveDelaySecs() const;

    virtual void clearSyncSourceBlacklist();

    /*
     * Implementation of the KillOpListenerInterface interrupt method so that we can wake up
     * threads blocked in awaitReplication() when a killOp command comes in.
     */
    virtual void interrupt(unsigned opId);

    /*
     * Implementation of the KillOpListenerInterface interruptAll method so that we can wake up
     * threads blocked in awaitReplication() when we kill all operations.
     */
    virtual void interruptAll();

    virtual ReplicationCoordinator::StatusAndDuration awaitReplication(
        const OperationContext* txn, const OpTime& ts, const WriteConcernOptions& writeConcern);

    virtual ReplicationCoordinator::StatusAndDuration awaitReplicationOfLastOpForClient(
        const OperationContext* txn, const WriteConcernOptions& writeConcern);

    virtual Status stepDown(OperationContext* txn,
                            bool force,
                            const Milliseconds& waitTime,
                            const Milliseconds& stepdownTime);

    virtual bool isMasterForReportingPurposes();

    virtual bool canAcceptWritesForDatabase(const StringData& dbName);

    virtual Status checkIfWriteConcernCanBeSatisfied(const WriteConcernOptions& writeConcern) const;

    virtual Status checkCanServeReadsFor(OperationContext* txn,
                                         const NamespaceString& ns,
                                         bool slaveOk);

    virtual bool shouldIgnoreUniqueIndex(const IndexDescriptor* idx);

    virtual Status setLastOptimeForSlave(const OID& rid, const OpTime& ts);

    virtual void setMyLastOptime(const OpTime& ts);

    virtual void resetMyLastOptime();

    virtual void setMyHeartbeatMessage(const std::string& msg);

    virtual OpTime getMyLastOptime() const;

    virtual OID getElectionId();

    virtual OID getMyRID() const;

    virtual int getMyId() const;

    virtual bool setFollowerMode(const MemberState& newState);

    virtual bool isWaitingForApplierToDrain();

    virtual void signalDrainComplete(OperationContext* txn);

    virtual void signalUpstreamUpdater();

    virtual bool prepareReplSetUpdatePositionCommand(BSONObjBuilder* cmdBuilder);

    virtual void prepareReplSetUpdatePositionCommandHandshakes(std::vector<BSONObj>* handshakes);

    virtual Status processReplSetGetStatus(BSONObjBuilder* result);

    virtual void fillIsMasterForReplSet(IsMasterResponse* result);

    virtual void appendSlaveInfoData(BSONObjBuilder* result);

    virtual void processReplSetGetConfig(BSONObjBuilder* result);

    virtual Status setMaintenanceMode(bool activate);

    virtual bool getMaintenanceMode();

    virtual Status processReplSetSyncFrom(const HostAndPort& target, BSONObjBuilder* resultObj);

    virtual Status processReplSetFreeze(int secs, BSONObjBuilder* resultObj);

    virtual Status processHeartbeat(const ReplSetHeartbeatArgs& args,
                                    ReplSetHeartbeatResponse* response);

    virtual Status processReplSetReconfig(OperationContext* txn,
                                          const ReplSetReconfigArgs& args,
                                          BSONObjBuilder* resultObj);

    virtual Status processReplSetInitiate(OperationContext* txn,
                                          const BSONObj& configObj,
                                          BSONObjBuilder* resultObj);

    virtual Status processReplSetGetRBID(BSONObjBuilder* resultObj);

    virtual void incrementRollbackID();

    virtual Status processReplSetFresh(const ReplSetFreshArgs& args, BSONObjBuilder* resultObj);

    virtual Status processReplSetElect(const ReplSetElectArgs& args, BSONObjBuilder* response);

    virtual Status processReplSetUpdatePosition(const UpdatePositionArgs& updates);

    virtual Status processHandshake(OperationContext* txn, const HandshakeArgs& handshake);

    virtual bool buildsIndexes();

    virtual std::vector<HostAndPort> getHostsWrittenTo(const OpTime& op);

    virtual std::vector<HostAndPort> getOtherNodesInReplSet() const;

    virtual WriteConcernOptions getGetLastErrorDefault();

    virtual Status checkReplEnabledForCommand(BSONObjBuilder* result);

    virtual bool isReplEnabled() const;

    virtual HostAndPort chooseNewSyncSource(const OpTime& lastOpTimeFetched);

    virtual void blacklistSyncSource(const HostAndPort& host, Date_t until);

    virtual void resetLastOpTimeFromOplog(OperationContext* txn);

    virtual bool shouldChangeSyncSource(const HostAndPort& currentSource);


    virtual void summarizeAsHtml(ReplSetHtmlSummary* s);

    // ================== Test support API ===================

    /**
     * If called after startReplication(), blocks until all asynchronous
     * activities associated with replication start-up complete.
     */
    void waitForStartUpComplete();

    /**
     * Gets the replica set configuration in use by the node.
     */
    ReplicaSetConfig getReplicaSetConfig_forTest();

    /**
     * Simple wrapper around _setLastOptime_inlock to make it easier to test.
     */
    Status setLastOptime_forTest(const OID& rid, const OpTime& ts);

private:
    /**
     * Configuration states for a replica set node.
     *
     * Transition diagram:
     *
     * PreStart ------------------> ReplicationDisabled
     *    |
     *    |
     *    v
     * StartingUp -------> Uninitialized <------> Initiating
     *         \                     ^               |
     *          -------              |               |
     *                 |             |               |
     *                 v             v               |
     * Reconfig <---> Steady <----> HBReconfig       |
     *                    ^                          /
     *                    |                         /
     *                     \                       /
     *                      -----------------------
     */
    enum ConfigState {
        kConfigPreStart,
        kConfigStartingUp,
        kConfigReplicationDisabled,
        kConfigUninitialized,
        kConfigSteady,
        kConfigInitiating,
        kConfigReconfiguring,
        kConfigHBReconfiguring
    };

    /**
     * Type describing actions to take after a change to the MemberState _memberState.
     */
    enum PostMemberStateUpdateAction {
        kActionNone,
        kActionCloseAllConnections,  // Also indicates that we should clear sharding state.
        kActionChooseNewSyncSource,
        kActionWinElection
    };

    // Struct that holds information about clients waiting for replication.
    struct WaiterInfo;

    // Struct that holds information about nodes in this replication group, mainly used for
    // tracking replication progress for write concern satisfaction.
    struct SlaveInfo {
        OpTime opTime;            // Our last known OpTime that this slave has replicated to.
        HostAndPort hostAndPort;  // Client address of the slave.
        int memberId;  // Id of the node in the replica set config, or -1 if we're not a replSet.
        OID rid;       // RID of the node.
        bool self;     // Whether this SlaveInfo stores the information about ourself
        SlaveInfo() : memberId(-1), self(false) {}
    };

    typedef std::vector<SlaveInfo> SlaveInfoVector;

    typedef std::vector<ReplicationExecutor::CallbackHandle> HeartbeatHandles;

    /**
     * Looks up the SlaveInfo in _slaveInfo associated with the given RID and returns a pointer
     * to it, or returns NULL if there is no SlaveInfo with the given RID.
     */
    SlaveInfo* _findSlaveInfoByRID_inlock(const OID& rid);

    /**
     * Looks up the SlaveInfo in _slaveInfo associated with the given member ID and returns a
     * pointer to it, or returns NULL if there is no SlaveInfo with the given member ID.
     */
    SlaveInfo* _findSlaveInfoByMemberID_inlock(int memberID);

    /**
     * Adds the given SlaveInfo to _slaveInfo and wakes up any threads waiting for replication
     * that now have their write concern satisfied.  Only valid to call in master/slave setups.
     */
    void _addSlaveInfo_inlock(const SlaveInfo& slaveInfo);

    /**
     * Updates the item in _slaveInfo pointed to by 'slaveInfo' with the given OpTime 'ts'
     * and wakes up any threads waiting for replication that now have their write concern
     * satisfied.
     */
    void _updateSlaveInfoOptime_inlock(SlaveInfo* slaveInfo, OpTime ts);

    /**
     * Returns the index into _slaveInfo where data corresponding to ourself is stored.
     * For more info on the rules about how we know where our entry is, see the comment for
     * _slaveInfo.
     */
    size_t _getMyIndexInSlaveInfo_inlock() const;

    /**
     * Helper method that removes entries from _slaveInfo if they correspond to a node
     * with a member ID that is not in the current replica set config.  Will always leave an
     * entry for ourself at the beginning of _slaveInfo, even if we aren't present in the
     * config.
     */
    void _updateSlaveInfoFromConfig_inlock();

    /**
     * Helper to update our saved config, cancel any pending heartbeats, and kick off sending
     * new heartbeats based on the new config.  Must *only* be called from within the
     * ReplicationExecutor context.
     *
     * Returns an action to be performed after unlocking _mutex, via
     * _performPostMemberStateUpdateAction.
     */
    PostMemberStateUpdateAction _setCurrentRSConfig_inlock(const ReplicaSetConfig& newConfig,
                                                           int myIndex);

    /**
     * Helper to wake waiters in _replicationWaiterList that are doneWaitingForReplication.
     */
    void _wakeReadyWaiters_inlock();

    /**
     * Helper method for setting/unsetting maintenance mode.  Scheduled by setMaintenanceMode()
     * to run in a global write lock in the replication executor thread.
     */
    void _setMaintenanceMode_helper(const ReplicationExecutor::CallbackData& cbData,
                                    bool activate,
                                    Status* result);

    /**
     * Helper method for retrieving maintenance mode.  Scheduled by getMaintenanceMode() to run
     * in the replication executor thread.
     */
    void _getMaintenanceMode_helper(const ReplicationExecutor::CallbackData& cbData,
                                    bool* maintenanceMode);

    /**
     * Bottom half of fillIsMasterForReplSet.
     */
    void _fillIsMasterForReplSet_finish(const ReplicationExecutor::CallbackData& cbData,
                                        IsMasterResponse* result);

    /**
     * Bottom half of processReplSetFresh.
     */
    void _processReplSetFresh_finish(const ReplicationExecutor::CallbackData& cbData,
                                     const ReplSetFreshArgs& args,
                                     BSONObjBuilder* response,
                                     Status* result);

    /**
     * Bottom half of processReplSetElect.
     */
    void _processReplSetElect_finish(const ReplicationExecutor::CallbackData& cbData,
                                     const ReplSetElectArgs& args,
                                     BSONObjBuilder* response,
                                     Status* result);

    /**
     * Bottom half of processReplSetFreeze.
     */
    void _processReplSetFreeze_finish(const ReplicationExecutor::CallbackData& cbData,
                                      int secs,
                                      BSONObjBuilder* response,
                                      Status* result);
    /*
     * Bottom half of clearSyncSourceBlacklist
     */
    void _clearSyncSourceBlacklist_finish(const ReplicationExecutor::CallbackData& cbData);

    /**
     * Scheduled to cause the ReplicationCoordinator to reconsider any state that might
     * need to change as a result of time passing - for instance becoming PRIMARY when a single
     * node replica set member's stepDown period ends.
     */
    void _handleTimePassing(const ReplicationExecutor::CallbackData& cbData);

    /**
     * Helper method for _awaitReplication that takes an already locked unique_lock and a
     * Timer for timing the operation which has been counting since before the lock was
     * acquired.
     */
    ReplicationCoordinator::StatusAndDuration _awaitReplication_inlock(
        const Timer* timer,
        boost::unique_lock<boost::mutex>* lock,
        const OperationContext* txn,
        const OpTime& ts,
        const WriteConcernOptions& writeConcern);

    /*
     * Returns true if the given writeConcern is satisfied up to "optime" or is unsatisfiable.
     */
    bool _doneWaitingForReplication_inlock(const OpTime& opTime,
                                           const WriteConcernOptions& writeConcern);

    /**
     * Helper for _doneWaitingForReplication_inlock that takes an integer write concern.
     */
    bool _haveNumNodesReachedOpTime_inlock(const OpTime& opTime, int numNodes);

    /**
     * Helper for _doneWaitingForReplication_inlock that takes a tag pattern representing a
     * named write concern mode.
     */
    bool _haveTaggedNodesReachedOpTime_inlock(const OpTime& opTime,
                                              const ReplicaSetTagPattern& tagPattern);

    Status _checkIfWriteConcernCanBeSatisfied_inlock(const WriteConcernOptions& writeConcern) const;

    /**
     * Triggers all callbacks that are blocked waiting for new heartbeat data
     * to decide whether or not to finish a step down.
     * Should only be called from executor callbacks.
     */
    void _signalStepDownWaitersFromCallback(const ReplicationExecutor::CallbackData& cbData);
    void _signalStepDownWaiters();

    /**
     * Helper for stepDown run within a ReplicationExecutor callback.  This method assumes
     * it is running within a global shared lock, and thus that no writes are going on at the
     * same time.
     */
    void _stepDownContinue(const ReplicationExecutor::CallbackData& cbData,
                           const ReplicationExecutor::EventHandle finishedEvent,
                           OperationContext* txn,
                           Date_t waitUntil,
                           Date_t stepdownUntil,
                           bool force,
                           Status* result);

    OID _getMyRID_inlock() const;

    int _getMyId_inlock() const;

    OpTime _getMyLastOptime_inlock() const;


    /**
     * Bottom half of setFollowerMode.
     *
     * May reschedule itself after the current election, so it is not sufficient to
     * wait for a callback scheduled to execute this method to complete.  Instead,
     * supply an event, "finishedSettingFollowerMode", and wait for that event to
     * be signaled.  Do not observe "*success" until after the event is signaled.
     */
    void _setFollowerModeFinish(const ReplicationExecutor::CallbackData& cbData,
                                const MemberState& newState,
                                const ReplicationExecutor::EventHandle& finishedSettingFollowerMode,
                                bool* success);

    /**
     * Helper method for updating our tracking of the last optime applied by a given node.
     * This is only valid to call on replica sets.
     */
    Status _setLastOptime_inlock(const UpdatePositionArgs::UpdateInfo& args);

    /**
     * Helper method for setMyLastOptime that takes in a unique lock on
     * _mutex.  The passed in lock must already be locked.  It is unspecified what state the
     * lock will be in after this method finishes.
     *
     * This function has the same rules for "ts" as setMyLastOptime(), unless
     * "isRollbackAllowed" is true.
     */
    void _setMyLastOptime_inlock(boost::unique_lock<boost::mutex>* lock,
                                 const OpTime& ts,
                                 bool isRollbackAllowed);

    /**
     * Schedules a heartbeat to be sent to "target" at "when". "targetIndex" is the index
     * into the replica set config members array that corresponds to the "target", or -1 if
     * "target" is not in _rsConfig.
     */
    void _scheduleHeartbeatToTarget(const HostAndPort& target, int targetIndex, Date_t when);

    /**
     * Processes each heartbeat response.
     *
     * Schedules additional heartbeats, triggers elections and step downs, etc.
     */
    void _handleHeartbeatResponse(const ReplicationExecutor::RemoteCommandCallbackData& cbData,
                                  int targetIndex);

    void _trackHeartbeatHandle(const StatusWith<ReplicationExecutor::CallbackHandle>& handle);

    void _untrackHeartbeatHandle(const ReplicationExecutor::CallbackHandle& handle);

    /**
     * Helper for _handleHeartbeatResponse.
     *
     * Updates the optime associated with the member at "memberIndex" in our config.
     */
    void _updateOpTimeFromHeartbeat_inlock(int memberIndex, OpTime optime);

    /**
     * Starts a heartbeat for each member in the current config.  Called within the executor
     * context.
     */
    void _startHeartbeats();

    /**
     * Cancels all heartbeats.  Called within executor context.
     */
    void _cancelHeartbeats();

    /**
     * Asynchronously sends a heartbeat to "target". "targetIndex" is the index
     * into the replica set config members array that corresponds to the "target", or -1 if
     * we don't have a valid replica set config.
     *
     * Scheduled by _scheduleHeartbeatToTarget.
     */
    void _doMemberHeartbeat(ReplicationExecutor::CallbackData cbData,
                            const HostAndPort& target,
                            int targetIndex);


    MemberState _getMemberState_inlock() const;

    /**
     * Returns the current replication mode. This method requires the caller to be holding
     * "_mutex" to be called safely.
     */
    Mode _getReplicationMode_inlock() const;

    /**
     * Starts loading the replication configuration from local storage, and if it is valid,
     * schedules a callback (of _finishLoadLocalConfig) to set it as the current replica set
     * config (sets _rsConfig and _thisMembersConfigIndex).
     * Returns true if it finishes loading the local config, which most likely means there
     * was no local config at all or it was invalid in some way, and false if there was a valid
     * config detected but more work is needed to set it as the local config (which will be
     * handled by the callback to _finishLoadLocalConfig).
     */
    bool _startLoadLocalConfig(OperationContext* txn);

    /**
     * Callback that finishes the work started in _startLoadLocalConfig and sets _rsConfigState
     * to kConfigSteady, so that we can begin processing heartbeats and reconfigs.
     */
    void _finishLoadLocalConfig(const ReplicationExecutor::CallbackData& cbData,
                                const ReplicaSetConfig& localConfig,
                                const StatusWith<OpTime>& lastOpTimeStatus);

    /**
     * Callback that finishes the work of processReplSetInitiate() inside the replication
     * executor context, in the event of a successful quorum check.
     */
    void _finishReplSetInitiate(const ReplicationExecutor::CallbackData& cbData,
                                const ReplicaSetConfig& newConfig,
                                int myIndex);

    /**
     * Callback that finishes the work of processReplSetReconfig inside the replication
     * executor context, in the event of a successful quorum check.
     */
    void _finishReplSetReconfig(const ReplicationExecutor::CallbackData& cbData,
                                const ReplicaSetConfig& newConfig,
                                int myIndex);

    /**
     * Changes _rsConfigState to newState, and notify any waiters.
     */
    void _setConfigState_inlock(ConfigState newState);

    /**
     * Updates the cached value, _memberState, to match _topCoord's reported
     * member state, from getMemberState().
     *
     * Returns an enum indicating what action to take after releasing _mutex, if any.
     * Call performPostMemberStateUpdateAction on the return value after releasing
     * _mutex.
     */
    PostMemberStateUpdateAction _updateMemberStateFromTopologyCoordinator_inlock();

    /**
     * Performs a post member-state update action.  Do not call while holding _mutex.
     */
    void _performPostMemberStateUpdateAction(PostMemberStateUpdateAction action);

    /**
     * Begins an attempt to elect this node.
     * Called after an incoming heartbeat changes this node's view of the set such that it
     * believes it can be elected PRIMARY.
     * For proper concurrency, must be called via a ReplicationExecutor callback.
     */
    void _startElectSelf();

    /**
     * Callback called when the FreshnessChecker has completed; checks the results and
     * decides whether to continue election proceedings.
     * finishEvh is an event that is signaled when election is complete.
     **/
    void _onFreshnessCheckComplete();

    /**
     * Callback called when the ElectCmdRunner has completed; checks the results and
     * decides whether to complete the election and change state to primary.
     * finishEvh is an event that is signaled when election is complete.
     **/
    void _onElectCmdRunnerComplete();

    /**
     * Callback called after a random delay, to prevent repeated election ties.
     */
    void _recoverFromElectionTie(const ReplicationExecutor::CallbackData& cbData);

    /**
     * Chooses a new sync source.  Must be scheduled as a callback.
     *
     * Calls into the Topology Coordinator, which uses its current view of the set to choose
     * the most appropriate sync source.
     */
    void _chooseNewSyncSource(const ReplicationExecutor::CallbackData& cbData,
                              const OpTime& lastOpTimeFetched,
                              HostAndPort* newSyncSource);

    /**
     * Adds 'host' to the sync source blacklist until 'until'. A blacklisted source cannot
     * be chosen as a sync source. Schedules a callback to unblacklist the sync source to be
     * run at 'until'.
     *
     * Must be scheduled as a callback.
     */
    void _blacklistSyncSource(const ReplicationExecutor::CallbackData& cbData,
                              const HostAndPort& host,
                              Date_t until);

    /**
     * Removes 'host' from the sync source blacklist. If 'host' isn't found, it's simply
     * ignored and no error is thrown.
     *
     * Must be scheduled as a callback.
     */
    void _unblacklistSyncSource(const ReplicationExecutor::CallbackData& cbData,
                                const HostAndPort& host);

    /**
     * Determines if a new sync source should be considered.
     *
     * Must be scheduled as a callback.
     */
    void _shouldChangeSyncSource(const ReplicationExecutor::CallbackData& cbData,
                                 const HostAndPort& currentSource,
                                 bool* shouldChange);

    /**
     * Schedules a request that the given host step down; logs any errors.
     */
    void _requestRemotePrimaryStepdown(const HostAndPort& target);

    void _heartbeatStepDownStart();

    /**
     * Completes a step-down of the current node triggered by a heartbeat.  Must
     * be run with a global shared or global exclusive lock.
     */
    void _heartbeatStepDownFinish(const ReplicationExecutor::CallbackData& cbData);

    /**
     * Schedules a replica set config change.
     */
    void _scheduleHeartbeatReconfig(const ReplicaSetConfig& newConfig);

    /**
     * Callback that continues a heartbeat-initiated reconfig after a running election
     * completes.
     */
    void _heartbeatReconfigAfterElectionCanceled(const ReplicationExecutor::CallbackData& cbData,
                                                 const ReplicaSetConfig& newConfig);

    /**
     * Method to write a configuration transmitted via heartbeat message to stable storage.
     */
    void _heartbeatReconfigStore(const ReplicaSetConfig& newConfig);

    /**
     * Conclusion actions of a heartbeat-triggered reconfiguration.
     */
    void _heartbeatReconfigFinish(const ReplicationExecutor::CallbackData& cbData,
                                  const ReplicaSetConfig& newConfig,
                                  StatusWith<int> myIndex);

    /**
     * Utility method that schedules or performs actions specified by a HeartbeatResponseAction
     * returned by a TopologyCoordinator::processHeartbeatResponse call with the given
     * value of "responseStatus".
     */
    void _handleHeartbeatResponseAction(const HeartbeatResponseAction& action,
                                        const StatusWith<ReplSetHeartbeatResponse>& responseStatus);

    /**
     * Bottom half of processHeartbeat(), which runs in the replication executor.
     */
    void _processHeartbeatFinish(const ReplicationExecutor::CallbackData& cbData,
                                 const ReplSetHeartbeatArgs& args,
                                 ReplSetHeartbeatResponse* response,
                                 Status* outStatus);

    void _summarizeAsHtml_finish(const ReplicationExecutor::CallbackData& cbData,
                                 ReplSetHtmlSummary* output);

    //
    // All member variables are labeled with one of the following codes indicating the
    // synchronization rules for accessing them.
    //
    // (R)  Read-only in concurrent operation; no synchronization required.
    // (S)  Self-synchronizing; access in any way from any context.
    // (PS) Pointer is read-only in concurrent operation, item pointed to is self-synchronizing;
    //      Access in any context.
    // (M)  Reads and writes guarded by _mutex
    // (X)  Reads and writes must be performed in a callback in _replExecutor
    // (MX) Must hold _mutex and be in a callback in _replExecutor to write; must either hold
    //      _mutex or be in a callback in _replExecutor to read.
    // (GX) Readable under a global intent lock.  Must either hold global lock in exclusive
    //      mode (MODE_X) or both hold global lock in shared mode (MODE_S) and be in executor
    //      context to write.
    // (I)  Independently synchronized, see member variable comment.

    // Protects member data of this ReplicationCoordinator.
    mutable boost::mutex _mutex;  // (S)

    // Handles to actively queued heartbeats.
    HeartbeatHandles _heartbeatHandles;  // (X)

    // When this node does not know itself to be a member of a config, it adds
    // every host that sends it a heartbeat request to this set, and also starts
    // sending heartbeat requests to that host.  This set is cleared whenever
    // a node discovers that it is a member of a config.
    unordered_set<HostAndPort> _seedList;  // (X)

    // Parsed command line arguments related to replication.
    const ReplSettings _settings;  // (R)

    // Mode of replication specified by _settings.
    const Mode _replMode;  // (R)

    // Pointer to the TopologyCoordinator owned by this ReplicationCoordinator.
    boost::scoped_ptr<TopologyCoordinator> _topCoord;  // (X)

    // Executor that drives the topology coordinator.
    ReplicationExecutor _replExecutor;  // (S)

    // Pointer to the ReplicationCoordinatorExternalState owned by this ReplicationCoordinator.
    boost::scoped_ptr<ReplicationCoordinatorExternalState> _externalState;  // (PS)

    // Thread that drives actions in the topology coordinator
    // Set in startReplication() and thereafter accessed in shutdown.
    boost::scoped_ptr<boost::thread> _topCoordDriverThread;  // (I)

    // Thread that is used to write new configs received via a heartbeat reconfig
    // to stable storage.  It is an error to change this if _inShutdown is true.
    boost::scoped_ptr<boost::thread> _heartbeatReconfigThread;  // (M)

    // Our RID, used to identify us to our sync source when sending replication progress
    // updates upstream.  Set once in startReplication() and then never modified again.
    OID _myRID;  // (M)

    // Rollback ID. Used to check if a rollback happened during some interval of time
    // TODO: ideally this should only change on rollbacks NOT on mongod restarts also.
    int _rbid;  // (M)

    // list of information about clients waiting on replication.  Does *not* own the
    // WaiterInfos.
    std::vector<WaiterInfo*> _replicationWaiterList;  // (M)

    // Set to true when we are in the process of shutting down replication.
    bool _inShutdown;  // (M)

    // Election ID of the last election that resulted in this node becoming primary.
    OID _electionId;  // (M)

    // Vector containing known information about each member (such as replication
    // progress and member ID) in our replica set or each member replicating from
    // us in a master-slave deployment.  In master/slave, the first entry is
    // guaranteed to correspond to ourself.  In replica sets where we don't have a
    // valid config or are in state REMOVED then the vector will be a single element
    // just with info about ourself.  In replica sets with a valid config the elements
    // will be in the same order as the members in the replica set config, thus
    // the entry for ourself will be at _thisMemberConfigIndex.
    SlaveInfoVector _slaveInfo;  // (M)

    // Current ReplicaSet state.
    MemberState _memberState;  // (MX)

    // True if we are waiting for the applier to finish draining.
    bool _isWaitingForDrainToComplete;  // (M)

    // Used to signal threads waiting for changes to _rsConfigState.
    boost::condition_variable _rsConfigStateChange;  // (M)

    // Represents the configuration state of the coordinator, which controls how and when
    // _rsConfig may change.  See the state transition diagram in the type definition of
    // ConfigState for details.
    ConfigState _rsConfigState;  // (M)

    // The current ReplicaSet configuration object, including the information about tag groups
    // that is used to satisfy write concern requests with named gle modes.
    ReplicaSetConfig _rsConfig;  // (MX)

    // This member's index position in the current config.
    int _selfIndex;  // (MX)

    // Vector of events that should be signaled whenever new heartbeat data comes in.
    std::vector<ReplicationExecutor::EventHandle> _stepDownWaiters;  // (X)

    // State for conducting an election of this node.
    // the presence of a non-null _freshnessChecker pointer indicates that an election is
    // currently in progress.  Only one election is allowed at once.
    boost::scoped_ptr<FreshnessChecker> _freshnessChecker;  // (X)

    boost::scoped_ptr<ElectCmdRunner> _electCmdRunner;  // (X)

    // Event that the election code will signal when the in-progress election completes.
    // Unspecified value when _freshnessChecker is NULL.
    ReplicationExecutor::EventHandle _electionFinishedEvent;  // (X)

    // Whether we slept last time we attempted an election but possibly tied with other nodes.
    bool _sleptLastElection;  // (X)

    // Flag that indicates whether writes to databases other than "local" are allowed.  Used to
    // answer the canAcceptWritesForDatabase() question.  Always true for standalone nodes and
    // masters in master-slave relationships.
    bool _canAcceptNonLocalWrites;  // (GX)

    // Flag that indicates whether reads from databases other than "local" are allowed.  Unlike
    // _canAcceptNonLocalWrites, above, this question is about admission control on secondaries,
    // and we do not require that its observers be strongly synchronized.  Accidentally
    // providing the prior value for a limited period of time is acceptable.  Also unlike
    // _canAcceptNonLocalWrites, its value is only meaningful on replica set secondaries.
    AtomicUInt32 _canServeNonLocalReads;  // (S)
};

}  // namespace repl
}  // namespace mongo
