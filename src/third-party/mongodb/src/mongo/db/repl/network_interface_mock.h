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

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <map>

#include "mongo/db/repl/replication_executor.h"
#include "mongo/util/time_support.h"

namespace mongo {
namespace repl {

/**
 * Mock network implementation for use in unit tests.
 *
 * To use, construct a new instance on the heap, and keep a pointer to it.  Pass
 * the pointer to the instance into the ReplicationExecutor constructor, transfering
 * ownership.  Start the executor's run() method in a separate thread, schedule the
 * work you want to test into the executor, then while the test is still going, iterate
 * through the ready network requests, servicing them and advancing time as needed.
 *
 * The mock has a fully virtualized notion of time and the the network.  When the
 * replication executor under test schedules a network operation, the startCommand
 * method of this class adds an entry to the _unscheduled queue for immediate consideration.
 * The test driver loop, when it examines the request, may schedule a response, ask the
 * interface to redeliver the request at a later virtual time, or to swallow the virtual
 * request until the end of the simulation.  The test driver loop can also instruct the
 * interface to run forward through virtual time until there are operations ready to
 * consider, via runUntil.
 *
 * The thread acting as the "network" and the executor run thread are highly synchronized
 * by this code, allowing for deterministic control of operation interleaving.
 */
class NetworkInterfaceMock : public ReplicationExecutor::NetworkInterface {
public:
    class NetworkOperation;
    typedef stdx::list<NetworkOperation> NetworkOperationList;
    typedef NetworkOperationList::iterator NetworkOperationIterator;

    NetworkInterfaceMock();
    virtual ~NetworkInterfaceMock();
    virtual std::string getDiagnosticString();

    ////////////////////////////////////////////////////////////////////////////////
    //
    // ReplicationExecutor::NetworkInterface methods
    //
    ////////////////////////////////////////////////////////////////////////////////

    virtual void startup();
    virtual void shutdown();
    virtual void waitForWork();
    virtual void waitForWorkUntil(Date_t when);
    virtual void signalWorkAvailable();
    virtual Date_t now();
    virtual void startCommand(const ReplicationExecutor::CallbackHandle& cbHandle,
                              const ReplicationExecutor::RemoteCommandRequest& request,
                              const RemoteCommandCompletionFn& onFinish);
    virtual void cancelCommand(const ReplicationExecutor::CallbackHandle& cbHandle);
    virtual void runCallbackWithGlobalExclusiveLock(
        const stdx::function<void(OperationContext*)>& callback);


    ////////////////////////////////////////////////////////////////////////////////
    //
    // Methods for simulating network operations and the passage of time.
    //
    // Methods in this section are to be called by the thread currently simulating
    // the network.
    //
    ////////////////////////////////////////////////////////////////////////////////

    /**
     * Causes the currently running (non-executor) thread to assume the mantle of the network
     * simulation thread.
     *
     * Call this before calling any of the other methods in this section.
     */
    void enterNetwork();

    /**
     * Causes the currently running thread to drop the mantle of "network simulation thread".
     *
     * Call this before calling any methods that might block waiting for the replciation
     * executor thread.
     */
    void exitNetwork();

    /**
     * Returns true if there are unscheduled network requests to be processed.
     */
    bool hasReadyRequests();

    /**
     * Gets the next unscheduled request to process, blocking until one is available.
     *
     * Will not return until the executor thread is blocked in waitForWorkUntil or waitForWork.
     */
    NetworkOperationIterator getNextReadyRequest();

    /**
     * Schedules "response" in response to "noi" at virtual time "when".
     */
    void scheduleResponse(NetworkOperationIterator noi,
                          Date_t when,
                          const ResponseStatus& response);

    /**
     * Swallows "noi", causing the network interface to not respond to it until
     * shutdown() is called.
     */
    void blackHole(NetworkOperationIterator noi);

    /**
     * Defers decision making on "noi" until virtual time "dontAskUntil".  Use
     * this when getNextReadyRequest() returns a request you want to deal with
     * after looking at other requests.
     */
    void requeueAt(NetworkOperationIterator noi, Date_t dontAskUntil);

    /**
     * Runs the simulator forward until now() == until or hasReadyRequests() is true.
     *
     * Will not return until the executor thread is blocked in waitForWorkUntil or waitForWork.
     */
    void runUntil(Date_t until);

    /**
     * Processes all ready, scheduled network operations.
     *
     * Will not return until the executor thread is blocked in waitForWorkUntil or waitForWork.
     */
    void runReadyNetworkOperations();

private:
    /**
     * Type used to identify which thread (network mock or executor) is currently executing.
     *
     * Values are used in a bitmask, as well.
     */
    enum ThreadType { kNoThread = 0, kExecutorThread = 1, kNetworkThread = 2 };

    /**
     * Returns the current virtualized time.
     */
    Date_t _now_inlock() const {
        return _now;
    }

    /**
     * Implementation of waitForWork*.
     */
    void _waitForWork_inlock(boost::unique_lock<boost::mutex>* lk);

    /**
     * Returns true if there are ready requests for the network thread to service.
     */
    bool _hasReadyRequests_inlock();

    /**
     * Returns true if the network thread could run right now.
     */
    bool _isNetworkThreadRunnable_inlock();

    /**
     * Returns true if the executor thread could run right now.
     */
    bool _isExecutorThreadRunnable_inlock();

    /**
     * Runs all ready network operations, called while holding "lk".  May drop and
     * reaquire "lk" several times, but will not return until the executor has blocked
     * in waitFor*.
     */
    void _runReadyNetworkOperations_inlock(boost::unique_lock<boost::mutex>* lk);

    // Mutex that synchronizes access to mutable data in this class and its subclasses.
    // Fields guarded by the mutex are labled (M), below, and those that are read-only
    // in multi-threaded execution, and so unsynchronized, are labeled (R).
    boost::mutex _mutex;

    // Condition signaled to indicate that the network processing thread should wake up.
    boost::condition_variable _shouldWakeNetworkCondition;  // (M)

    // Condition signaled to indicate that the executor run thread should wake up.
    boost::condition_variable _shouldWakeExecutorCondition;  // (M)

    // Bitmask indicating which threads are runnable.
    int _waitingToRunMask;  // (M)

    // Indicator of which thread, if any, is currently running.
    ThreadType _currentlyRunning;  // (M)

    // The current time reported by this instance of NetworkInterfaceMock.
    Date_t _now;  // (M)

    // Set to true by "startUp()"
    bool _hasStarted;  // (M)

    // Set to true by "shutDown()".
    bool _inShutdown;  // (M)

    // Next date that the executor expects to wake up at (due to a scheduleWorkAt() call).
    Date_t _executorNextWakeupDate;  // (M)

    // List of network operations whose responses haven't been scheduled or blackholed.  This is
    // where network requests are first queued.  It is sorted by
    // NetworkOperation::_nextConsiderationDate, which is set to now() when startCommand() is
    // called, and adjusted by requeueAt().
    NetworkOperationList _unscheduled;  // (M)

    // List of network operations that have been returned by getNextReadyRequest() but not
    // yet scheudled, black-holed or requeued.
    NetworkOperationList _processing;  // (M)

    // List of network operations whose responses have been scheduled but not delivered, sorted
    // by NetworkOperation::_responseDate.  These operations will have their responses delivered
    // when now() == getResponseDate().
    NetworkOperationList _scheduled;  // (M)

    // List of network operations that will not be responded to until shutdown() is called.
    NetworkOperationList _blackHoled;  // (M)

    // Pointer to the executor into which this mock is installed.  Used to signal the executor
    // when the clock changes.
    ReplicationExecutor* _executor;  // (R)
};

/**
 * Representation of an in-progress network operation.
 */
class NetworkInterfaceMock::NetworkOperation {
public:
    NetworkOperation();
    NetworkOperation(const ReplicationExecutor::CallbackHandle& cbHandle,
                     const ReplicationExecutor::RemoteCommandRequest& theRequest,
                     Date_t theRequestDate,
                     const RemoteCommandCompletionFn& onFinish);
    ~NetworkOperation();

    /**
     * Adjusts the stored virtual time at which this entry will be subject to consideration
     * by the test harness.
     */
    void setNextConsiderationDate(Date_t nextConsiderationDate);

    /**
     * Sets the response and thet virtual time at which it will be delivered.
     */
    void setResponse(Date_t responseDate, const ResponseStatus& response);

    /**
     * Predicate that returns true if cbHandle equals the executor's handle for this network
     * operation.  Used for searching lists of NetworkOperations.
     */
    bool isForCallback(const ReplicationExecutor::CallbackHandle& cbHandle) const {
        return cbHandle == _cbHandle;
    }

    /**
     * Gets the request that initiated this operation.
     */
    const ReplicationExecutor::RemoteCommandRequest& getRequest() const {
        return _request;
    }

    /**
     * Gets the virtual time at which the operation was started.
     */
    Date_t getRequestDate() const {
        return _requestDate;
    }

    /**
     * Gets the virtual time at which the test harness should next consider what to do
     * with this request.
     */
    Date_t getNextConsiderationDate() const {
        return _nextConsiderationDate;
    }

    /**
     * After setResponse() has been called, returns the virtual time at which
     * the response should be delivered.
     */
    Date_t getResponseDate() const {
        return _responseDate;
    }

    /**
     * Delivers the response, by invoking the onFinish callback passed into the constructor.
     */
    void finishResponse();

private:
    Date_t _requestDate;
    Date_t _nextConsiderationDate;
    Date_t _responseDate;
    ReplicationExecutor::CallbackHandle _cbHandle;
    ReplicationExecutor::RemoteCommandRequest _request;
    ResponseStatus _response;
    RemoteCommandCompletionFn _onFinish;
};

}  // namespace repl
}  // namespace mongo
