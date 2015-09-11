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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kNetwork

#include "mongo/platform/basic.h"

#include "mongo/client/replica_set_monitor.h"

#include <algorithm>
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>
#include <limits>

#include "mongo/db/server_options.h"
#include "mongo/client/connpool.h"
#include "mongo/client/replica_set_monitor_internal.h"
#include "mongo/util/background.h"
#include "mongo/util/concurrency/mutex.h"  // for StaticObserver
#include "mongo/util/debug_util.h"
#include "mongo/util/exit.h"
#include "mongo/util/log.h"
#include "mongo/util/string_map.h"
#include "mongo/util/timer.h"

#if 0  // enable this to ease debugging of this file.
#undef DEV
#define DEV if (true)

#undef LOG
#define LOG(x) log()
#endif

namespace mongo {

using std::numeric_limits;
using std::set;
using std::string;

namespace {
// Pull nested types to top-level scope
typedef ReplicaSetMonitor::IsMasterReply IsMasterReply;
typedef ReplicaSetMonitor::ScanState ScanState;
typedef ReplicaSetMonitor::ScanStatePtr ScanStatePtr;
typedef ReplicaSetMonitor::SetState SetState;
typedef ReplicaSetMonitor::SetStatePtr SetStatePtr;
typedef ReplicaSetMonitor::Refresher Refresher;
typedef Refresher::NextStep NextStep;
typedef ScanState::UnconfirmedReplies UnconfirmedReplies;
typedef SetState::Node Node;
typedef SetState::Nodes Nodes;

const double socketTimeoutSecs = 5;

/*  Replica Set Monitor shared state:
 *      If a program (such as one built with the C++ driver) exits (by either calling exit()
 *      or by returning from main()), static objects will be destroyed in the reverse order
 *      of their creation (within each translation unit (source code file)).  This makes it
 *      vital that the order be explicitly controlled within the source file so that destroyed
 *      objects never reference objects that have been destroyed earlier.
 *
 *      The order chosen below is intended to allow safe destruction in reverse order from
 *      construction order:
 *          setsLock                 -- mutex protecting _seedServers and _sets, destroyed last
 *          seedServers              -- list (map) of servers
 *          sets                     -- list (map) of ReplicaSetMonitors
 *          replicaSetMonitorWatcher -- background job to check Replica Set members
 *          staticObserver           -- sentinel to detect process termination
 *
 *      Related to:
 *          SERVER-8891 -- Simple client fail with segmentation fault in mongoclient library
 *
 *      Mutex locking order:
 *          Don't lock setsLock while holding any SetState::mutex. It is however safe to grab a
 *          SetState::mutex without holder setsLock, but then you can't grab setsLock until you
 *          release the SetState::mutex.
 */
mongo::mutex setsLock("ReplicaSetMonitor");
StringMap<set<HostAndPort>> seedServers;
StringMap<ReplicaSetMonitorPtr> sets;

// global background job responsible for checking every X amount of time
class ReplicaSetMonitorWatcher : public BackgroundJob {
public:
    ReplicaSetMonitorWatcher()
        : _monitorMutex("ReplicaSetMonitorWatcher::_safego"),
          _started(false),
          _stopRequested(false) {}

    ~ReplicaSetMonitorWatcher() {
        stop();

        // We relying on the fact that if the monitor was rerun again, wait will not hang
        // because _destroyingStatics will make the run method exit immediately.
        dassert(StaticObserver::_destroyingStatics);
        if (running()) {
            wait();
        }
    }

    virtual string name() const {
        return "ReplicaSetMonitorWatcher";
    }

    void safeGo() {
        scoped_lock lk(_monitorMutex);
        if (_started)
            return;

        _started = true;
        _stopRequested = false;

        go();
    }

    /**
     * Stops monitoring the sets and wait for the monitoring thread to terminate.
     */
    void stop() {
        scoped_lock sl(_monitorMutex);
        _stopRequested = true;
        _stopRequestedCV.notify_one();
    }

protected:
    void run() {
        log() << "starting";  // includes thread name in output

        // Added only for patching timing problems in test. Remove after tests
        // are fixed - see 392b933598668768bf12b1e41ad444aa3548d970.
        // Should not be needed after SERVER-7533 gets implemented and tests start
        // using it.
        if (!inShutdown() && !StaticObserver::_destroyingStatics) {
            scoped_lock sl(_monitorMutex);
            _stopRequestedCV.timed_wait(sl.boost(), boost::posix_time::seconds(10));
        }

        while (!inShutdown() && !StaticObserver::_destroyingStatics) {
            {
                scoped_lock sl(_monitorMutex);
                if (_stopRequested) {
                    break;
                }
            }

            try {
                checkAllSets();
            } catch (std::exception& e) {
                error() << "check failed: " << e.what();
            } catch (...) {
                error() << "unknown error";
            }

            scoped_lock sl(_monitorMutex);
            if (_stopRequested) {
                break;
            }

            _stopRequestedCV.timed_wait(sl.boost(), boost::posix_time::seconds(10));
        }
    }

    void checkAllSets() {
        // make a copy so we can quickly unlock setsLock
        StringMap<ReplicaSetMonitorPtr> setsCopy;
        {
            scoped_lock lk(setsLock);
            setsCopy = sets;
        }

        for (StringMap<ReplicaSetMonitorPtr>::const_iterator it = setsCopy.begin();
             it != setsCopy.end();
             ++it) {
            LOG(1) << "checking replica set: " << it->first;
            ReplicaSetMonitorPtr m = it->second;

            m->startOrContinueRefresh().refreshAll();

            const int numFails = m->getConsecutiveFailedScans();
            if (numFails >= ReplicaSetMonitor::maxConsecutiveFailedChecks) {
                log() << "Replica set " << m->getName() << " was down for " << numFails
                      << " checks in a row. Stopping polled monitoring of the set.";

                // locks setsLock
                ReplicaSetMonitor::remove(m->getName(), false);
            }
        }
    }

    // protects _started, _stopRequested
    mongo::mutex _monitorMutex;
    bool _started;

    boost::condition _stopRequestedCV;
    bool _stopRequested;
} replicaSetMonitorWatcher;

StaticObserver staticObserver;

//
// Helpers for stl algorithms
//

bool isMaster(const Node& node) {
    return node.isMaster;
}
bool compareLatencies(const Node* lhs, const Node* rhs) {
    // NOTE: this automatically compares Node::unknownLatency worse than all others.
    return lhs->latencyMicros < rhs->latencyMicros;
}

bool hostsEqual(const Node& lhs, const HostAndPort& rhs) {
    return lhs.host == rhs;
}

// Allows comparing two Nodes, or a HostAndPort and a Node.
// NOTE: the two HostAndPort overload is only needed to support extra checks in some STL
// implementations. For simplicity, no comparator should be used with collections of just
// HostAndPort.
struct CompareHosts {
    bool operator()(const Node& lhs, const Node& rhs) {
        return lhs.host < rhs.host;
    }
    bool operator()(const Node& lhs, const HostAndPort& rhs) {
        return lhs.host < rhs;
    }
    bool operator()(const HostAndPort& lhs, const Node& rhs) {
        return lhs < rhs.host;
    }
    bool operator()(const HostAndPort& lhs, const HostAndPort& rhs) {
        return lhs < rhs;
    }
} compareHosts;  // like an overloaded function, but able to pass to stl algorithms

// The following structs should be treated as functions returning a UnaryPredicate.
// Usage example: std::find_if(nodes.begin(), nodes.end(), hostIs(someHost));
// They all hold their constructor argument by reference.

struct hostIs {
    explicit hostIs(const HostAndPort& host) : _host(host) {}
    bool operator()(const HostAndPort& host) {
        return host == _host;
    }
    bool operator()(const Node& node) {
        return node.host == _host;
    }
    const HostAndPort& _host;
};

struct hostNotIn {
    explicit hostNotIn(const std::set<HostAndPort>& hosts) : _hosts(hosts) {}
    bool operator()(const HostAndPort& host) {
        return !_hosts.count(host);
    }
    bool operator()(const Node& node) {
        return !_hosts.count(node.host);
    }
    const std::set<HostAndPort>& _hosts;
};
}  // namespace

// At 1 check every 10 seconds, 30 checks takes 5 minutes
int ReplicaSetMonitor::maxConsecutiveFailedChecks = 30;

// Defaults to random selection as required by the spec
bool ReplicaSetMonitor::useDeterministicHostSelection = false;

ReplicaSetMonitor::ReplicaSetMonitor(StringData name, const std::set<HostAndPort>& seeds)
    : _state(boost::make_shared<SetState>(name, seeds)) {
    LogstreamBuilder lsb = log();
    lsb << "starting new replica set monitor for replica set " << name << " with seeds ";
    for (std::set<HostAndPort>::const_iterator it = seeds.begin(); it != seeds.end(); ++it) {
        if (it != seeds.begin())
            lsb << ',';
        lsb << *it;
    }
}

HostAndPort ReplicaSetMonitor::getHostOrRefresh(const ReadPreferenceSetting& criteria) {
    {
        boost::mutex::scoped_lock lk(_state->mutex);
        HostAndPort out = _state->getMatchingHost(criteria);
        if (!out.empty())
            return out;
    }

    {
        Refresher refresher = startOrContinueRefresh();
        HostAndPort out = refresher.refreshUntilMatches(criteria);
        if (!out.empty() || refresher.startedNewScan())
            return out;
    }

    // We didn't find any matching hosts and the scan we just finished may have stale data from
    // before we joined. Therefore we should participate in a new scan to make sure all hosts
    // are contacted at least once (possibly by other threads) before this function gives up.

    return startOrContinueRefresh().refreshUntilMatches(criteria);
}

HostAndPort ReplicaSetMonitor::getMasterOrUassert() {
    const ReadPreferenceSetting masterOnly(ReadPreference_PrimaryOnly, TagSet());
    HostAndPort master = getHostOrRefresh(masterOnly);
    uassert(10009,
            str::stream() << "ReplicaSetMonitor no master found for set: " << getName(),
            !master.empty());
    return master;
}

Refresher ReplicaSetMonitor::startOrContinueRefresh() {
    boost::mutex::scoped_lock lk(_state->mutex);

    Refresher out(_state);
    DEV _state->checkInvariants();
    return out;
}

void ReplicaSetMonitor::failedHost(const HostAndPort& host) {
    boost::mutex::scoped_lock lk(_state->mutex);
    Node* node = _state->findNode(host);
    if (node)
        node->markFailed();
    DEV _state->checkInvariants();
}

bool ReplicaSetMonitor::isPrimary(const HostAndPort& host) const {
    boost::mutex::scoped_lock lk(_state->mutex);
    Node* node = _state->findNode(host);
    return node ? node->isMaster : false;
}

bool ReplicaSetMonitor::isHostUp(const HostAndPort& host) const {
    boost::mutex::scoped_lock lk(_state->mutex);
    Node* node = _state->findNode(host);
    return node ? node->isUp : false;
}

int ReplicaSetMonitor::getConsecutiveFailedScans() const {
    boost::mutex::scoped_lock lk(_state->mutex);
    return _state->consecutiveFailedScans;
}

std::string ReplicaSetMonitor::getName() const {
    // name is const so don't need to lock
    return _state->name;
}

std::string ReplicaSetMonitor::getServerAddress() const {
    boost::mutex::scoped_lock lk(_state->mutex);
    return _state->getServerAddress();
}

bool ReplicaSetMonitor::contains(const HostAndPort& host) const {
    boost::mutex::scoped_lock lk(_state->mutex);
    return _state->seedNodes.count(host);
}

void ReplicaSetMonitor::createIfNeeded(const string& name, const set<HostAndPort>& servers) {
    LOG(3) << "ReplicaSetMonitor::createIfNeeded " << name;
    scoped_lock lk(setsLock);
    ReplicaSetMonitorPtr& m = sets[name];
    if (!m)
        m = boost::make_shared<ReplicaSetMonitor>(name, servers);

    replicaSetMonitorWatcher.safeGo();
}

ReplicaSetMonitorPtr ReplicaSetMonitor::get(const std::string& name, const bool createFromSeed) {
    LOG(3) << "ReplicaSetMonitor::get " << name;
    scoped_lock lk(setsLock);
    StringMap<ReplicaSetMonitorPtr>::const_iterator i = sets.find(name);
    if (i != sets.end()) {
        return i->second;
    }
    if (createFromSeed) {
        StringMap<set<HostAndPort>>::const_iterator j = seedServers.find(name);
        if (j != seedServers.end()) {
            LOG(4) << "Creating ReplicaSetMonitor from cached address";
            ReplicaSetMonitorPtr& m = sets[name];
            invariant(!m);
            m.reset(new ReplicaSetMonitor(name, j->second));
            replicaSetMonitorWatcher.safeGo();
            return m;
        }
    }
    return ReplicaSetMonitorPtr();
}

set<string> ReplicaSetMonitor::getAllTrackedSets() {
    set<string> activeSets;
    scoped_lock lk(setsLock);
    for (StringMap<ReplicaSetMonitorPtr>::const_iterator it = sets.begin(); it != sets.end();
         ++it) {
        activeSets.insert(it->first);
    }
    return activeSets;
}

void ReplicaSetMonitor::remove(const string& name, bool clearSeedCache) {
    LOG(2) << "Removing ReplicaSetMonitor for " << name << " from replica set table"
           << (clearSeedCache ? " and the seed cache" : "");

    scoped_lock lk(setsLock);
    const StringMap<ReplicaSetMonitorPtr>::const_iterator setIt = sets.find(name);
    if (setIt != sets.end()) {
        if (!clearSeedCache) {
            // Save list of current set members so that the monitor can be rebuilt if needed.
            const ReplicaSetMonitorPtr& rsm = setIt->second;
            boost::mutex::scoped_lock lk(rsm->_state->mutex);
            seedServers[name] = rsm->_state->seedNodes;
        }
        sets.erase(setIt);
    }

    if (clearSeedCache) {
        seedServers.erase(name);
    }

    // Kill all pooled ReplicaSetConnections for this set. They will not function correctly
    // after we kill the ReplicaSetMonitor.
    // TODO we may only need to do this if clearSeedCache is true.
    pool.removeHost(name);
}

void ReplicaSetMonitor::setConfigChangeHook(ConfigChangeHook hook) {
    massert(13610, "ConfigChangeHook already specified", !SetState::configChangeHook);
    SetState::configChangeHook = hook;
}

// TODO move to correct order with non-statics before pushing
void ReplicaSetMonitor::appendInfo(BSONObjBuilder& bsonObjBuilder) const {
    boost::mutex::scoped_lock lk(_state->mutex);

    // NOTE: the format here must be consistent for backwards compatibility
    BSONArrayBuilder hosts(bsonObjBuilder.subarrayStart("hosts"));
    for (unsigned i = 0; i < _state->nodes.size(); i++) {
        const Node& node = _state->nodes[i];

        BSONObjBuilder builder;
        builder.append("addr", node.host.toString());
        builder.append("ok", node.isUp);
        builder.append("ismaster", node.isMaster);  // intentionally not camelCase
        builder.append("hidden", false);            // we don't keep hidden nodes in the set
        builder.append("secondary", node.isUp && !node.isMaster);

        int32_t pingTimeMillis = 0;
        if (node.latencyMicros / 1000 > numeric_limits<int32_t>::max()) {
            // In particular, Node::unknownLatency does not fit in an int32.
            pingTimeMillis = numeric_limits<int32_t>::max();
        } else {
            pingTimeMillis = node.latencyMicros / 1000;
        }
        builder.append("pingTimeMillis", pingTimeMillis);

        if (!node.tags.isEmpty()) {
            builder.append("tags", node.tags);
        }

        hosts.append(builder.obj());
    }
    hosts.done();
}

void ReplicaSetMonitor::cleanup() {
    // Call cancel first, in case the RSMW was never started.
    replicaSetMonitorWatcher.cancel();
    replicaSetMonitorWatcher.stop();
    replicaSetMonitorWatcher.wait();
    scoped_lock lock(setsLock);
    sets = StringMap<ReplicaSetMonitorPtr>();
    seedServers = StringMap<set<HostAndPort>>();
}

Refresher::Refresher(const SetStatePtr& setState)
    : _set(setState), _scan(setState->currentScan), _startedNewScan(false) {
    if (_scan)
        return;  // participate in in-progress scan

    LOG(2) << "Starting new refresh of replica set " << _set->name;
    _scan = startNewScan(_set.get());
    _set->currentScan = _scan;
    _startedNewScan = true;
}

Refresher::NextStep Refresher::getNextStep() {
    if (_scan != _set->currentScan)
        return NextStep(NextStep::DONE);  // No longer the current scan.

    // Wait for all dispatched hosts to return before trying any fallback hosts.
    if (_scan->hostsToScan.empty() && !_scan->waitingFor.empty())
        return NextStep(NextStep::WAIT);

    // If we haven't yet found a master, try contacting unconfirmed hosts
    if (_scan->hostsToScan.empty() && !_scan->foundUpMaster) {
        _scan->enqueAllUntriedHosts(_scan->possibleNodes, _set->rand);
        _scan->possibleNodes.clear();
    }

    if (_scan->hostsToScan.empty()) {
        // We've tried all hosts we can, so nothing more to do in this round.
        if (!_scan->foundUpMaster) {
            warning() << "No primary detected for set " << _set->name;

            // Since we've talked to everyone we could but still didn't find a primary, we
            // do the best we can, and assume all unconfirmedReplies are actually from nodes
            // in the set (we've already confirmed that they think they are). This is
            // important since it allows us to bootstrap to a usable state even if we are
            // unable to talk to a master while starting up. As soon as we are able to
            // contact a master, we will remove any nodes that it doesn't think are part of
            // the set, undoing the damage we cause here.

            // NOTE: we don't modify seedNodes or notify about set membership change in this
            // case since it hasn't been confirmed by a master.
            for (UnconfirmedReplies::iterator it = _scan->unconfirmedReplies.begin();
                 it != _scan->unconfirmedReplies.end();
                 ++it) {
                _set->findOrCreateNode(it->host)->update(*it);
            }
        }

        if (_scan->foundAnyUpNodes) {
            _set->consecutiveFailedScans = 0;
        } else {
            _set->consecutiveFailedScans++;
            log() << "All nodes for set " << _set->name << " are down. "
                  << "This has happened for " << _set->consecutiveFailedScans
                  << " checks in a row. Polling will stop after "
                  << maxConsecutiveFailedChecks - _set->consecutiveFailedScans
                  << " more failed checks";
        }

        _set->currentScan.reset();  // Makes sure all other Refreshers in this round return DONE
        return NextStep(NextStep::DONE);
    }

    // Pop and return the next hostToScan.
    HostAndPort host = _scan->hostsToScan.front();
    _scan->hostsToScan.pop_front();
    _scan->waitingFor.insert(host);
    _scan->triedHosts.insert(host);
    return NextStep(NextStep::CONTACT_HOST, host);
}

void Refresher::receivedIsMaster(const HostAndPort& from,
                                 int64_t latencyMicros,
                                 const BSONObj& replyObj) {
    // Be careful: all return paths must call either failedHost or cv.notify_all!
    _scan->waitingFor.erase(from);

    const IsMasterReply reply(from, latencyMicros, replyObj);
    // Handle various failure cases
    if (!reply.ok) {
        failedHost(from);
        return;
    }

    if (reply.setName != _set->name) {
        warning() << "node: " << from << " isn't a part of set: " << _set->name
                  << " ismaster: " << replyObj;
        failedHost(from);
        return;
    }

    if (reply.isMaster) {
        const bool stalePrimary = !receivedIsMasterFromMaster(reply);
        if (stalePrimary) {
            log() << "node " << from << " believes it is primary, but its election id of "
                  << reply.electionId << " is older than the most recent election id"
                  << " for this set, " << _set->maxElectionId;
            failedHost(from);
            return;
        }
    }

    if (_scan->foundUpMaster) {
        // We only update a Node if a master has confirmed it is in the set.
        _set->updateNodeIfInNodes(reply);
    } else {
        receivedIsMasterBeforeFoundMaster(reply);
        _scan->unconfirmedReplies.push_back(reply);
    }

    // _set->nodes may still not have any nodes with isUp==true, but we have at least found a
    // connectible host that is that claims to be in the set.
    _scan->foundAnyUpNodes = true;

    // TODO consider only notifying if we've updated a node or we've emptied waitingFor.
    _set->cv.notify_all();

    DEV _set->checkInvariants();
}

void Refresher::failedHost(const HostAndPort& host) {
    _scan->waitingFor.erase(host);

    // Failed hosts can't pass criteria, so the only way they'd effect the _refreshUntilMatches
    // loop is if it was the last host we were waitingFor.
    if (_scan->waitingFor.empty())
        _set->cv.notify_all();

    Node* node = _set->findNode(host);
    if (node)
        node->markFailed();
}

ScanStatePtr Refresher::startNewScan(const SetState* set) {
    const ScanStatePtr scan = boost::make_shared<ScanState>();

    // The heuristics we use in deciding the order to contact hosts are designed to find a
    // master as quickly as possible. This is because we can't use any hosts we find until
    // we either get the latest set of members from a master or talk to all possible hosts
    // without finding a master.

    // TODO It might make sense to check down nodes first if the last seen master is still
    // marked as up.

    int upNodes = 0;
    for (Nodes::const_iterator it(set->nodes.begin()), end(set->nodes.end()); it != end; ++it) {
        if (it->isUp) {
            // scan the nodes we think are up first
            scan->hostsToScan.push_front(it->host);
            upNodes++;
        } else {
            scan->hostsToScan.push_back(it->host);
        }
    }

    // shuffle the queue, but keep "up" nodes at the front
    std::random_shuffle(scan->hostsToScan.begin(), scan->hostsToScan.begin() + upNodes, set->rand);
    std::random_shuffle(scan->hostsToScan.begin() + upNodes, scan->hostsToScan.end(), set->rand);

    if (!set->lastSeenMaster.empty()) {
        // move lastSeenMaster to front of queue
        std::stable_partition(
            scan->hostsToScan.begin(), scan->hostsToScan.end(), hostIs(set->lastSeenMaster));
    }

    return scan;
}

bool Refresher::receivedIsMasterFromMaster(const IsMasterReply& reply) {
    invariant(reply.isMaster);

    if (reply.electionId.isSet()) {
        if (_set->maxElectionId.isSet() && _set->maxElectionId.compare(reply.electionId) > 0) {
            return false;
        }
        _set->maxElectionId = reply.electionId;
    }

    // Mark all nodes as not master. We will mark ourself as master before releasing the lock.
    // NOTE: we use a "last-wins" policy if multiple hosts claim to be master.
    for (size_t i = 0; i < _set->nodes.size(); i++) {
        _set->nodes[i].isMaster = false;
    }

    // Check if the master agrees with our current list of nodes.
    // REMINDER: both _set->nodes and reply.normalHosts are sorted.
    if (_set->nodes.size() != reply.normalHosts.size() ||
        !std::equal(
            _set->nodes.begin(), _set->nodes.end(), reply.normalHosts.begin(), hostsEqual)) {
        LOG(2) << "Adjusting nodes in our view of replica set " << _set->name
               << " based on master reply: " << reply.raw;

        // remove non-members from _set->nodes
        _set->nodes.erase(
            std::remove_if(_set->nodes.begin(), _set->nodes.end(), hostNotIn(reply.normalHosts)),
            _set->nodes.end());

        // add new members to _set->nodes
        for (std::set<HostAndPort>::const_iterator it = reply.normalHosts.begin();
             it != reply.normalHosts.end();
             ++it) {
            _set->findOrCreateNode(*it);
        }

        // replace hostToScan queue with untried normal hosts. can both add and remove
        // hosts from the queue.
        _scan->hostsToScan.clear();
        _scan->enqueAllUntriedHosts(reply.normalHosts, _set->rand);

        if (!_scan->waitingFor.empty()) {
            // make sure we don't wait for any hosts that aren't considered members
            std::set<HostAndPort> newWaitingFor;
            std::set_intersection(reply.normalHosts.begin(),
                                  reply.normalHosts.end(),
                                  _scan->waitingFor.begin(),
                                  _scan->waitingFor.end(),
                                  std::inserter(newWaitingFor, newWaitingFor.end()));
            _scan->waitingFor.swap(newWaitingFor);
        }
    }

    if (reply.normalHosts != _set->seedNodes) {
        const string oldAddr = _set->getServerAddress();
        _set->seedNodes = reply.normalHosts;

        // LogLevel can be pretty low, since replica set reconfiguration should be pretty rare
        // and we want to record our changes
        log() << "changing hosts to " << _set->getServerAddress() << " from " << oldAddr;

        if (SetState::configChangeHook) {
            // call from a separate thread to avoid blocking and holding lock while potentially
            // going over the network
            boost::thread bg(SetState::configChangeHook, _set->name, _set->getServerAddress());
            bg.detach();
        }
    }

    // Update other nodes's information based on replies we've already seen
    for (UnconfirmedReplies::iterator it = _scan->unconfirmedReplies.begin();
         it != _scan->unconfirmedReplies.end();
         ++it) {
        // this ignores replies from hosts not in _set->nodes (as modified above)
        _set->updateNodeIfInNodes(*it);
    }
    _scan->unconfirmedReplies.clear();

    _scan->foundUpMaster = true;
    _set->lastSeenMaster = reply.host;

    return true;
}

void Refresher::receivedIsMasterBeforeFoundMaster(const IsMasterReply& reply) {
    invariant(!reply.isMaster);
    // This function doesn't alter _set at all. It only modifies the work queue in _scan.

    // Add everyone this host claims is in the set to possibleNodes.
    _scan->possibleNodes.insert(reply.normalHosts.begin(), reply.normalHosts.end());

    // If this node thinks the primary is someone we haven't tried, make that the next
    // hostToScan.
    if (!reply.primary.empty() && !_scan->triedHosts.count(reply.primary)) {
        std::deque<HostAndPort>::iterator it = std::stable_partition(
            _scan->hostsToScan.begin(), _scan->hostsToScan.end(), hostIs(reply.primary));

        if (it == _scan->hostsToScan.begin()) {
            // reply.primary wasn't in hostsToScan
            _scan->hostsToScan.push_front(reply.primary);
        }
    }
}

HostAndPort Refresher::_refreshUntilMatches(const ReadPreferenceSetting* criteria) {
    boost::mutex::scoped_lock lk(_set->mutex);
    while (true) {
        if (criteria) {
            HostAndPort out = _set->getMatchingHost(*criteria);
            if (!out.empty())
                return out;
        }

        const NextStep ns = getNextStep();
        switch (ns.step) {
            case NextStep::DONE:
                DEV _set->checkInvariants();
                // getNextStep may have updated nodes if no master was found.
                return criteria ? _set->getMatchingHost(*criteria) : HostAndPort();

            case NextStep::WAIT:  // TODO consider treating as DONE for refreshAll
                DEV _set->checkInvariants();
                _set->cv.wait(lk);
                continue;

            case NextStep::CONTACT_HOST: {
                BSONObj reply;  // empty on error
                int64_t pingMicros = 0;

                DEV _set->checkInvariants();
                lk.unlock();  // relocked after attempting to call isMaster
                try {
                    ScopedDbConnection conn(ConnectionString(ns.host), socketTimeoutSecs);
                    bool ignoredOutParam = false;
                    Timer timer;
                    conn->isMaster(ignoredOutParam, &reply);
                    pingMicros = timer.micros();
                    conn.done();  // return to pool on success.
                } catch (...) {
                    reply = BSONObj();  // should be a no-op but want to be sure
                }
                lk.lock();

                // Ignore the reply and return if we are no longer the current scan. This might
                // happen if it was decided that the host we were contacting isn't part of the set.
                if (_scan != _set->currentScan)
                    return criteria ? _set->getMatchingHost(*criteria) : HostAndPort();

                if (reply.isEmpty())
                    failedHost(ns.host);
                else
                    receivedIsMaster(ns.host, pingMicros, reply);
            }
        }
    }
}

void IsMasterReply::parse(const BSONObj& obj) {
    try {
        raw = obj.getOwned();  // don't use obj again after this line

        ok = raw["ok"].trueValue();
        if (!ok)
            return;

        setName = raw["setName"].str();
        hidden = raw["hidden"].trueValue();
        secondary = raw["secondary"].trueValue();

        // hidden nodes can't be master, even if they claim to be.
        isMaster = !hidden && raw["ismaster"].trueValue();

        if (isMaster && raw.hasField("electionId")) {
            electionId = raw["electionId"].OID();
        }

        const string primaryString = raw["primary"].str();
        primary = primaryString.empty() ? HostAndPort() : HostAndPort(primaryString);

        // both hosts and passives, but not arbiters, are considered "normal hosts"
        normalHosts.clear();
        BSONForEach(host, raw.getObjectField("hosts")) {
            normalHosts.insert(HostAndPort(host.String()));
        }
        BSONForEach(host, raw.getObjectField("passives")) {
            normalHosts.insert(HostAndPort(host.String()));
        }

        tags = raw.getObjectField("tags");
    } catch (const std::exception& e) {
        ok = false;
        log() << "exception while parsing isMaster reply: " << e.what() << " " << obj;
    }
}

const int64_t Node::unknownLatency = numeric_limits<int64_t>::max();

bool Node::matches(const ReadPreference& pref) const {
    if (!isUp)
        return false;

    if (pref == ReadPreference_PrimaryOnly) {
        return isMaster;
    }

    if (pref == ReadPreference_SecondaryOnly) {
        if (isMaster)
            return false;
    }

    return true;
}

bool Node::matches(const BSONObj& tag) const {
    BSONForEach(tagCriteria, tag) {
        if (this->tags[tagCriteria.fieldNameStringData()] != tagCriteria)
            return false;
    }

    return true;
}

void Node::update(const IsMasterReply& reply) {
    invariant(host == reply.host);
    invariant(reply.ok);

    LOG(3) << "Updating host " << host << " based on ismaster reply: " << reply.raw;

    // Nodes that are hidden or neither master or secondary are considered down since we can't
    // send any operations to them.
    isUp = !reply.hidden && (reply.isMaster || reply.secondary);
    isMaster = reply.isMaster;

    // save a copy if unchanged
    if (!tags.binaryEqual(reply.tags))
        tags = reply.tags.getOwned();

    if (reply.latencyMicros >= 0) {  // TODO upper bound?
        if (latencyMicros == unknownLatency) {
            latencyMicros = reply.latencyMicros;
        } else {
            // update latency with smoothed moving average (1/4th the delta)
            latencyMicros += (reply.latencyMicros - latencyMicros) / 4;
        }
    }
}

ReplicaSetMonitor::ConfigChangeHook SetState::configChangeHook;

SetState::SetState(StringData name, const std::set<HostAndPort>& seedNodes)
    : name(name.toString()),
      consecutiveFailedScans(0),
      seedNodes(seedNodes),
      latencyThresholdMicros(serverGlobalParams.defaultLocalThresholdMillis * 1000),
      rand(int64_t(time(0))),
      roundRobin(0) {
    uassert(13642, "Replica set seed list can't be empty", !seedNodes.empty());

    if (name.empty())
        warning() << "Replica set name empty, first node: " << *(seedNodes.begin());

    // This adds the seed hosts to nodes, but they aren't usable for anything except seeding a
    // scan until we start a scan and either find a master or contact all hosts without finding
    // one.
    // WARNING: if seedNodes is ever changed to not imply sorted iteration, you will need to
    // sort nodes after this loop.
    for (std::set<HostAndPort>::const_iterator it = seedNodes.begin(); it != seedNodes.end();
         ++it) {
        nodes.push_back(Node(*it));
    }

    DEV checkInvariants();
}

HostAndPort SetState::getMatchingHost(const ReadPreferenceSetting& criteria) const {
    switch (criteria.pref) {
        // "Prefered" read preferences are defined in terms of other preferences
        case ReadPreference_PrimaryPreferred: {
            HostAndPort out =
                getMatchingHost(ReadPreferenceSetting(ReadPreference_PrimaryOnly, criteria.tags));
            // NOTE: the spec says we should use the primary even if tags don't match
            if (!out.empty())
                return out;
            return getMatchingHost(
                ReadPreferenceSetting(ReadPreference_SecondaryOnly, criteria.tags));
        }

        case ReadPreference_SecondaryPreferred: {
            HostAndPort out =
                getMatchingHost(ReadPreferenceSetting(ReadPreference_SecondaryOnly, criteria.tags));
            if (!out.empty())
                return out;
            // NOTE: the spec says we should use the primary even if tags don't match
            return getMatchingHost(
                ReadPreferenceSetting(ReadPreference_PrimaryOnly, criteria.tags));
        }

        case ReadPreference_PrimaryOnly: {
            // NOTE: isMaster implies isUp
            Nodes::const_iterator it = std::find_if(nodes.begin(), nodes.end(), isMaster);
            if (it == nodes.end())
                return HostAndPort();
            return it->host;
        }

        // The difference between these is handled by Node::matches
        case ReadPreference_SecondaryOnly:
        case ReadPreference_Nearest: {
            BSONForEach(tagElem, criteria.tags.getTagBSON()) {
                uassert(16358, "Tags should be a BSON object", tagElem.isABSONObj());
                BSONObj tag = tagElem.Obj();

                std::vector<const Node*> matchingNodes;
                for (size_t i = 0; i < nodes.size(); i++) {
                    if (nodes[i].matches(criteria.pref) && nodes[i].matches(tag)) {
                        matchingNodes.push_back(&nodes[i]);
                    }
                }

                // don't do more complicated selection if not needed
                if (matchingNodes.empty())
                    continue;
                if (matchingNodes.size() == 1)
                    return matchingNodes.front()->host;

                // order by latency and don't consider hosts further than a threshold from the
                // closest.
                std::sort(matchingNodes.begin(), matchingNodes.end(), compareLatencies);
                for (size_t i = 1; i < matchingNodes.size(); i++) {
                    int64_t distance =
                        matchingNodes[i]->latencyMicros - matchingNodes[0]->latencyMicros;
                    if (distance >= latencyThresholdMicros) {
                        // this node and all remaining ones are too far away
                        matchingNodes.erase(matchingNodes.begin() + i, matchingNodes.end());
                        break;
                    }
                }

                // of the remaining nodes, pick one at random (or use round-robin)
                if (ReplicaSetMonitor::useDeterministicHostSelection) {
                    // only in tests
                    return matchingNodes[roundRobin++ % matchingNodes.size()]->host;
                } else {
                    // normal case
                    return matchingNodes[rand.nextInt32(matchingNodes.size())]->host;
                };
            }

            return HostAndPort();
        }
        default:
            uassert(16337, "Unknown read preference", false);
            break;
    }
}

Node* SetState::findNode(const HostAndPort& host) {
    const Nodes::iterator it = std::lower_bound(nodes.begin(), nodes.end(), host, compareHosts);
    if (it == nodes.end() || it->host != host)
        return NULL;

    return &(*it);
}

Node* SetState::findOrCreateNode(const HostAndPort& host) {
    // This is insertion sort, but N is currently guaranteed to be <= 12 (although this class
    // must function correctly even with more nodes). If we lift that restriction, we may need
    // to consider alternate algorithms.
    Nodes::iterator it = std::lower_bound(nodes.begin(), nodes.end(), host, compareHosts);
    if (it == nodes.end() || it->host != host) {
        LOG(2) << "Adding node " << host << " to our view of replica set " << name;
        it = nodes.insert(it, Node(host));
    }
    return &(*it);
}

void SetState::updateNodeIfInNodes(const IsMasterReply& reply) {
    Node* node = findNode(reply.host);
    if (!node) {
        LOG(2) << "Skipping application of ismaster reply from " << reply.host
               << " since it isn't a confirmed member of set " << name;
        return;
    }

    node->update(reply);
}

std::string SetState::getServerAddress() const {
    StringBuilder ss;
    if (!name.empty())
        ss << name << "/";

    for (std::set<HostAndPort>::const_iterator it = seedNodes.begin(); it != seedNodes.end();
         ++it) {
        if (it != seedNodes.begin())
            ss << ",";
        it->append(ss);
    }

    return ss.str();
}

void SetState::checkInvariants() const {
    bool foundMaster = false;
    for (size_t i = 0; i < nodes.size(); i++) {
        // no empty hosts
        invariant(!nodes[i].host.empty());

        if (nodes[i].isMaster) {
            // masters must be up
            invariant(nodes[i].isUp);

            // at most one master
            invariant(!foundMaster);
            foundMaster = true;

            // if we have a master it should be the same as lastSeenMaster
            invariant(nodes[i].host == lastSeenMaster);
        }

        // should never end up with negative latencies
        invariant(nodes[i].latencyMicros >= 0);

        // nodes must be sorted by host with no-dupes
        invariant(i == 0 || (nodes[i - 1].host < nodes[i].host));
    }

    // nodes should be a (non-strict) superset of the seedNodes
    invariant(std::includes(
        nodes.begin(), nodes.end(), seedNodes.begin(), seedNodes.end(), compareHosts));

    if (currentScan) {
        // hostsToScan can't have dups or hosts already in triedHosts.
        std::set<HostAndPort> cantSee = currentScan->triedHosts;
        for (std::deque<HostAndPort>::const_iterator it = currentScan->hostsToScan.begin();
             it != currentScan->hostsToScan.end();
             ++it) {
            invariant(!cantSee.count(*it));
            cantSee.insert(*it);  // make sure we don't see this again
        }

        // We should only be waitingFor hosts that are in triedHosts
        invariant(std::includes(currentScan->triedHosts.begin(),
                                currentScan->triedHosts.end(),
                                currentScan->waitingFor.begin(),
                                currentScan->waitingFor.end()));

        // We should only have unconfirmedReplies if we haven't found a master yet
        invariant(!currentScan->foundUpMaster || currentScan->unconfirmedReplies.empty());
    }
}

template <typename Container>
void ScanState::enqueAllUntriedHosts(const Container& container, PseudoRandom& rand) {
    invariant(hostsToScan.empty());  // because we don't try to dedup hosts already in the queue.

    // no std::copy_if before c++11
    for (typename Container::const_iterator it(container.begin()), end(container.end()); it != end;
         ++it) {
        if (!triedHosts.count(*it)) {
            hostsToScan.push_back(*it);
        }
    }
    std::random_shuffle(hostsToScan.begin(), hostsToScan.end(), rand);
}
}
