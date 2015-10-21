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

#include "mongo/platform/basic.h"

#include "mongo/db/repl/replication_executor.h"

#include <limits>

#include "mongo/util/assert_util.h"
#include "mongo/util/mongoutils/str.h"

namespace mongo {
namespace repl {

namespace {
stdx::function<void()> makeNoExcept(const stdx::function<void()>& fn);
}  // namespace

const ReplicationExecutor::Milliseconds ReplicationExecutor::kNoTimeout(-1);
const Date_t ReplicationExecutor::kNoExpirationDate(-1);

ReplicationExecutor::ReplicationExecutor(NetworkInterface* netInterface, int64_t prngSeed)
    : _random(prngSeed),
      _networkInterface(netInterface),
      _totalEventWaiters(0),
      _inShutdown(false),
      _dblockWorkers(
          threadpool::ThreadPool::DoNotStartThreadsTag(), 1, "replCallbackWithGlobalLock-"),
      _nextId(0) {}

ReplicationExecutor::~ReplicationExecutor() {}

std::string ReplicationExecutor::getDiagnosticString() {
    boost::lock_guard<boost::mutex> lk(_mutex);
    return _getDiagnosticString_inlock();
}

std::string ReplicationExecutor::_getDiagnosticString_inlock() const {
    str::stream output;
    output << "ReplicationExecutor";
    output << " networkInProgress:" << _networkInProgressQueue.size();
    output << " exclusiveInProgress:" << _exclusiveLockInProgressQueue.size();
    output << " sleeperQueue:" << _sleepersQueue.size();
    output << " ready:" << _readyQueue.size();
    output << " free:" << _freeQueue.size();
    output << " unsignaledEvents:" << _unsignaledEvents.size();
    output << " eventWaiters:" << _totalEventWaiters;
    output << " shuttingDown:" << _inShutdown;
    output << " networkInterface:" << _networkInterface->getDiagnosticString();
    return output;
}

Date_t ReplicationExecutor::now() {
    return _networkInterface->now();
}

void ReplicationExecutor::run() {
    setThreadName("ReplicationExecutor");
    _networkInterface->startup();
    _dblockWorkers.startThreads();
    std::pair<WorkItem, CallbackHandle> work;
    while ((work = getWork()).first.callback) {
        {
            boost::lock_guard<boost::mutex> lk(_terribleExLockSyncMutex);
            const Status inStatus = work.first.isCanceled
                ? Status(ErrorCodes::CallbackCanceled, "Callback canceled")
                : Status::OK();
            makeNoExcept(
                stdx::bind(work.first.callback, CallbackData(this, work.second, inStatus)))();
        }
        signalEvent(work.first.finishedEvent);
    }
    finishShutdown();
    _networkInterface->shutdown();
}

void ReplicationExecutor::shutdown() {
    // Correct shutdown needs to:
    // * Disable future work queueing.
    // * drain all of the unsignaled events, sleepers, and ready queue, by running those
    //   callbacks with a "shutdown" or "canceled" status.
    // * Signal all threads blocked in waitForEvent, and wait for them to return from that method.
    boost::lock_guard<boost::mutex> lk(_mutex);
    _inShutdown = true;

    _readyQueue.splice(_readyQueue.end(), _exclusiveLockInProgressQueue);
    _readyQueue.splice(_readyQueue.end(), _networkInProgressQueue);
    _readyQueue.splice(_readyQueue.end(), _sleepersQueue);
    for (EventList::iterator event = _unsignaledEvents.begin(); event != _unsignaledEvents.end();
         ++event) {
        _readyQueue.splice(_readyQueue.end(), event->waiters);
    }
    for (WorkQueue::iterator readyWork = _readyQueue.begin(); readyWork != _readyQueue.end();
         ++readyWork) {
        readyWork->isCanceled = true;
    }
    _networkInterface->signalWorkAvailable();
}

void ReplicationExecutor::finishShutdown() {
    _dblockWorkers.join();
    boost::unique_lock<boost::mutex> lk(_mutex);
    invariant(_inShutdown);
    invariant(_exclusiveLockInProgressQueue.empty());
    invariant(_readyQueue.empty());
    invariant(_sleepersQueue.empty());

    while (!_unsignaledEvents.empty()) {
        EventList::iterator event = _unsignaledEvents.begin();
        invariant(event->waiters.empty());
        signalEvent_inlock(EventHandle(event, ++_nextId));
    }

    while (_totalEventWaiters > 0)
        _noMoreWaitingThreads.wait(lk);

    invariant(_exclusiveLockInProgressQueue.empty());
    invariant(_readyQueue.empty());
    invariant(_sleepersQueue.empty());
    invariant(_unsignaledEvents.empty());
}

void ReplicationExecutor::maybeNotifyShutdownComplete_inlock() {
    if (_totalEventWaiters == 0)
        _noMoreWaitingThreads.notify_all();
}

StatusWith<ReplicationExecutor::EventHandle> ReplicationExecutor::makeEvent() {
    boost::lock_guard<boost::mutex> lk(_mutex);
    return makeEvent_inlock();
}

StatusWith<ReplicationExecutor::EventHandle> ReplicationExecutor::makeEvent_inlock() {
    if (_inShutdown)
        return StatusWith<EventHandle>(ErrorCodes::ShutdownInProgress, "Shutdown in progress");

    if (_signaledEvents.empty())
        _signaledEvents.push_back(Event());
    const EventList::iterator iter = _signaledEvents.begin();
    invariant(iter->waiters.empty());
    iter->generation++;
    iter->isSignaled = false;
    _unsignaledEvents.splice(_unsignaledEvents.end(), _signaledEvents, iter);
    return StatusWith<EventHandle>(EventHandle(iter, ++_nextId));
}

void ReplicationExecutor::signalEvent(const EventHandle& event) {
    boost::lock_guard<boost::mutex> lk(_mutex);
    signalEvent_inlock(event);
}

void ReplicationExecutor::signalEvent_inlock(const EventHandle& event) {
    invariant(!event._iter->isSignaled);
    invariant(event._iter->generation == event._generation);
    event._iter->isSignaled = true;
    _signaledEvents.splice(_signaledEvents.end(), _unsignaledEvents, event._iter);
    if (!event._iter->waiters.empty()) {
        _readyQueue.splice(_readyQueue.end(), event._iter->waiters);
        _networkInterface->signalWorkAvailable();
    }
    event._iter->isSignaledCondition->notify_all();
}

StatusWith<ReplicationExecutor::CallbackHandle> ReplicationExecutor::onEvent(
    const EventHandle& event, const CallbackFn& work) {
    boost::lock_guard<boost::mutex> lk(_mutex);
    invariant(event.isValid());
    invariant(event._generation <= event._iter->generation);
    WorkQueue* queue = &_readyQueue;
    if (event._generation == event._iter->generation && !event._iter->isSignaled) {
        queue = &event._iter->waiters;
    } else {
        queue = &_readyQueue;
    }
    return enqueueWork_inlock(queue, work);
}

void ReplicationExecutor::waitForEvent(const EventHandle& event) {
    boost::unique_lock<boost::mutex> lk(_mutex);
    invariant(event.isValid());
    ++_totalEventWaiters;
    while ((event._generation == event._iter->generation) && !event._iter->isSignaled) {
        event._iter->isSignaledCondition->wait(lk);
    }
    --_totalEventWaiters;
    maybeNotifyShutdownComplete_inlock();
}

static void remoteCommandFinished(const ReplicationExecutor::CallbackData& cbData,
                                  const ReplicationExecutor::RemoteCommandCallbackFn& cb,
                                  const ReplicationExecutor::RemoteCommandRequest& request,
                                  const ResponseStatus& response) {
    if (cbData.status.isOK()) {
        cb(ReplicationExecutor::RemoteCommandCallbackData(
            cbData.executor, cbData.myHandle, request, response));
    } else {
        cb(ReplicationExecutor::RemoteCommandCallbackData(
            cbData.executor, cbData.myHandle, request, ResponseStatus(cbData.status)));
    }
}

static void remoteCommandFailedEarly(const ReplicationExecutor::CallbackData& cbData,
                                     const ReplicationExecutor::RemoteCommandCallbackFn& cb,
                                     const ReplicationExecutor::RemoteCommandRequest& request) {
    invariant(!cbData.status.isOK());
    cb(ReplicationExecutor::RemoteCommandCallbackData(
        cbData.executor, cbData.myHandle, request, ResponseStatus(cbData.status)));
}

void ReplicationExecutor::_finishRemoteCommand(const RemoteCommandRequest& request,
                                               const ResponseStatus& response,
                                               const CallbackHandle& cbHandle,
                                               const uint64_t expectedHandleGeneration,
                                               const RemoteCommandCallbackFn& cb) {
    const WorkQueue::iterator iter = cbHandle._iter;
    boost::lock_guard<boost::mutex> lk(_mutex);
    if (_inShutdown) {
        return;
    }
    if (expectedHandleGeneration != iter->generation) {
        return;
    }
    iter->callback =
        stdx::bind(remoteCommandFinished, stdx::placeholders::_1, cb, request, response);
    _readyQueue.splice(_readyQueue.end(), _networkInProgressQueue, iter);
}

StatusWith<ReplicationExecutor::CallbackHandle> ReplicationExecutor::scheduleRemoteCommand(
    const RemoteCommandRequest& request, const RemoteCommandCallbackFn& cb) {
    RemoteCommandRequest scheduledRequest = request;
    if (request.timeout == kNoTimeout) {
        scheduledRequest.expirationDate = kNoExpirationDate;
    } else {
        scheduledRequest.expirationDate =
            _networkInterface->now() + scheduledRequest.timeout.total_milliseconds();
    }
    boost::lock_guard<boost::mutex> lk(_mutex);
    StatusWith<CallbackHandle> handle = enqueueWork_inlock(
        &_networkInProgressQueue,
        stdx::bind(remoteCommandFailedEarly, stdx::placeholders::_1, cb, scheduledRequest));
    if (handle.isOK()) {
        handle.getValue()._iter->isNetworkOperation = true;
        _networkInterface->startCommand(handle.getValue(),
                                        scheduledRequest,
                                        stdx::bind(&ReplicationExecutor::_finishRemoteCommand,
                                                   this,
                                                   scheduledRequest,
                                                   stdx::placeholders::_1,
                                                   handle.getValue(),
                                                   handle.getValue()._iter->generation,
                                                   cb));
    }
    return handle;
}

StatusWith<ReplicationExecutor::CallbackHandle> ReplicationExecutor::scheduleWork(
    const CallbackFn& work) {
    boost::lock_guard<boost::mutex> lk(_mutex);
    _networkInterface->signalWorkAvailable();
    return enqueueWork_inlock(&_readyQueue, work);
}

StatusWith<ReplicationExecutor::CallbackHandle> ReplicationExecutor::scheduleWorkAt(
    Date_t when, const CallbackFn& work) {
    boost::lock_guard<boost::mutex> lk(_mutex);
    WorkQueue temp;
    StatusWith<CallbackHandle> cbHandle = enqueueWork_inlock(&temp, work);
    if (!cbHandle.isOK())
        return cbHandle;
    cbHandle.getValue()._iter->readyDate = when;
    WorkQueue::iterator insertBefore = _sleepersQueue.begin();
    while (insertBefore != _sleepersQueue.end() && insertBefore->readyDate <= when)
        ++insertBefore;
    _sleepersQueue.splice(insertBefore, temp, temp.begin());
    return cbHandle;
}

void ReplicationExecutor::doOperationWithGlobalExclusiveLock(OperationContext* txn,
                                                             const CallbackHandle& cbHandle) {
    boost::unique_lock<boost::mutex> lk(_mutex);
    if (_inShutdown)
        return;
    const WorkQueue::iterator iter = cbHandle._iter;
    const uint64_t generation = iter->generation;
    invariant(generation == cbHandle._generation);
    WorkItem work = *iter;
    iter->callback = CallbackFn();
    _freeQueue.splice(_freeQueue.begin(), _exclusiveLockInProgressQueue, iter);
    lk.unlock();
    {
        boost::lock_guard<boost::mutex> terribleLock(_terribleExLockSyncMutex);
        work.callback(CallbackData(this,
                                   cbHandle,
                                   (work.isCanceled
                                        ? Status(ErrorCodes::CallbackCanceled, "Callback canceled")
                                        : Status::OK()),
                                   txn));
    }
    lk.lock();
    signalEvent_inlock(work.finishedEvent);
}

StatusWith<ReplicationExecutor::CallbackHandle>
ReplicationExecutor::scheduleWorkWithGlobalExclusiveLock(const CallbackFn& work) {
    boost::lock_guard<boost::mutex> lk(_mutex);
    StatusWith<CallbackHandle> handle = enqueueWork_inlock(&_exclusiveLockInProgressQueue, work);
    if (handle.isOK()) {
        const stdx::function<void(OperationContext*)> doOp =
            stdx::bind(&ReplicationExecutor::doOperationWithGlobalExclusiveLock,
                       this,
                       stdx::placeholders::_1,
                       handle.getValue());
        _dblockWorkers.schedule(makeNoExcept(stdx::bind(
            &NetworkInterface::runCallbackWithGlobalExclusiveLock, _networkInterface.get(), doOp)));
    }
    return handle;
}

void ReplicationExecutor::cancel(const CallbackHandle& cbHandle) {
    boost::unique_lock<boost::mutex> lk(_mutex);
    if (cbHandle._iter->generation != cbHandle._generation) {
        return;
    }
    cbHandle._iter->isCanceled = true;
    if (cbHandle._iter->isNetworkOperation) {
        lk.unlock();
        _networkInterface->cancelCommand(cbHandle);
    }
}

void ReplicationExecutor::wait(const CallbackHandle& cbHandle) {
    waitForEvent(cbHandle._finishedEvent);
}

std::pair<ReplicationExecutor::WorkItem, ReplicationExecutor::CallbackHandle>
ReplicationExecutor::getWork() {
    boost::unique_lock<boost::mutex> lk(_mutex);
    while (true) {
        const Date_t now = _networkInterface->now();
        Date_t nextWakeupDate = scheduleReadySleepers_inlock(now);
        if (!_readyQueue.empty()) {
            break;
        } else if (_inShutdown) {
            return std::make_pair(WorkItem(), CallbackHandle());
        }
        lk.unlock();
        if (nextWakeupDate == Date_t(~0ULL)) {
            _networkInterface->waitForWork();
        } else {
            _networkInterface->waitForWorkUntil(nextWakeupDate);
        }
        lk.lock();
    }
    const CallbackHandle cbHandle(_readyQueue.begin());
    const WorkItem work = *cbHandle._iter;
    _readyQueue.begin()->callback = CallbackFn();
    _freeQueue.splice(_freeQueue.begin(), _readyQueue, _readyQueue.begin());
    return std::make_pair(work, cbHandle);
}

int64_t ReplicationExecutor::nextRandomInt64(int64_t limit) {
    return _random.nextInt64(limit);
}

Date_t ReplicationExecutor::scheduleReadySleepers_inlock(const Date_t now) {
    WorkQueue::iterator iter = _sleepersQueue.begin();
    while ((iter != _sleepersQueue.end()) && (iter->readyDate <= now)) {
        ++iter;
    }
    _readyQueue.splice(_readyQueue.end(), _sleepersQueue, _sleepersQueue.begin(), iter);
    if (iter == _sleepersQueue.end()) {
        // indicate no sleeper to wait for
        return Date_t(~0ULL);
    }
    return iter->readyDate;
}

StatusWith<ReplicationExecutor::CallbackHandle> ReplicationExecutor::enqueueWork_inlock(
    WorkQueue* queue, const CallbackFn& callback) {
    invariant(callback);
    StatusWith<EventHandle> event = makeEvent_inlock();
    if (!event.isOK())
        return StatusWith<CallbackHandle>(event.getStatus());

    if (_freeQueue.empty())
        _freeQueue.push_front(WorkItem());
    const WorkQueue::iterator iter = _freeQueue.begin();
    iter->generation++;
    iter->callback = callback;
    iter->finishedEvent = event.getValue();
    iter->readyDate = Date_t();
    iter->isCanceled = false;
    queue->splice(queue->end(), _freeQueue, iter);
    return StatusWith<CallbackHandle>(CallbackHandle(iter));
}

ReplicationExecutor::EventHandle::EventHandle(const EventList::iterator& iter, uint64_t id)
    : _iter(iter), _generation(iter->generation), _id(id) {}

ReplicationExecutor::CallbackHandle::CallbackHandle(const WorkQueue::iterator& iter)
    : _iter(iter), _generation(iter->generation), _finishedEvent(iter->finishedEvent) {}

ReplicationExecutor::CallbackData::CallbackData(ReplicationExecutor* theExecutor,
                                                const CallbackHandle& theHandle,
                                                const Status& theStatus,
                                                OperationContext* theTxn)
    : executor(theExecutor), myHandle(theHandle), status(theStatus), txn(theTxn) {}

ReplicationExecutor::RemoteCommandRequest::RemoteCommandRequest()
    : timeout(kNoTimeout), expirationDate(kNoExpirationDate) {}

ReplicationExecutor::RemoteCommandRequest::RemoteCommandRequest(const HostAndPort& theTarget,
                                                                const std::string& theDbName,
                                                                const BSONObj& theCmdObj,
                                                                const Milliseconds timeoutMillis)
    : target(theTarget), dbname(theDbName), cmdObj(theCmdObj), timeout(timeoutMillis) {
    if (timeoutMillis == kNoTimeout) {
        expirationDate = kNoExpirationDate;
    }
}

std::string ReplicationExecutor::RemoteCommandRequest::getDiagnosticString() {
    str::stream out;
    out << "RemoteCommand -- target:" << target.toString() << " db:" << dbname;

    if (expirationDate != kNoExpirationDate)
        out << " expDate:" << expirationDate.toString();

    out << " cmd:" << cmdObj.getOwned().toString();
    return out;
}

ReplicationExecutor::RemoteCommandCallbackData::RemoteCommandCallbackData(
    ReplicationExecutor* theExecutor,
    const CallbackHandle& theHandle,
    const RemoteCommandRequest& theRequest,
    const ResponseStatus& theResponse)
    : executor(theExecutor), myHandle(theHandle), request(theRequest), response(theResponse) {}

ReplicationExecutor::WorkItem::WorkItem()
    : generation(0U), isNetworkOperation(false), isCanceled(false) {}

ReplicationExecutor::Event::Event()
    : generation(0), isSignaled(false), isSignaledCondition(new boost::condition_variable) {}

ReplicationExecutor::NetworkInterface::NetworkInterface() {}
ReplicationExecutor::NetworkInterface::~NetworkInterface() {}

namespace {

void callNoExcept(const stdx::function<void()>& fn) {
    try {
        fn();
    } catch (...) {
        std::terminate();
    }
}

stdx::function<void()> makeNoExcept(const stdx::function<void()>& fn) {
    return stdx::bind(callNoExcept, fn);
}

}  // namespace

}  // namespace repl
}  // namespace mongo
