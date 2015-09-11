// thread_pool.h

/*    Copyright 2009 10gen Inc.
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

#pragma once

#include <list>

#include <boost/noncopyable.hpp>
#include <boost/thread/condition.hpp>

#include "mongo/stdx/functional.h"
#include "mongo/util/concurrency/mutex.h"

namespace mongo {

namespace threadpool {
class Worker;

typedef stdx::function<void(void)> Task;  // nullary function or functor

// exported to the mongo namespace
class ThreadPool : boost::noncopyable {
public:
    struct DoNotStartThreadsTag {};

    explicit ThreadPool(int nThreads = 8, const std::string& threadNamePrefix = "");
    explicit ThreadPool(const DoNotStartThreadsTag&,
                        int nThreads = 8,
                        const std::string& threadNamePrefix = "");

    // blocks until all tasks are complete (tasks_remaining() == 0)
    // You should not call schedule while in the destructor
    ~ThreadPool();

    // Launches the worker threads; call exactly once, if and only if
    // you used the DoNotStartThreadsTag form of the constructor.
    void startThreads();

    // blocks until all tasks are complete (tasks_remaining() == 0)
    // does not prevent new tasks from being scheduled so could wait forever.
    // Also, new tasks could be scheduled after this returns.
    void join();

    // task will be copied a few times so make sure it's relatively cheap
    void schedule(Task task);

    // Helpers that wrap schedule and stdx::bind.
    // Functor and args will be copied a few times so make sure it's relatively cheap
    template <typename F, typename A>
    void schedule(F f, A a) {
        schedule(stdx::bind(f, a));
    }
    template <typename F, typename A, typename B>
    void schedule(F f, A a, B b) {
        schedule(stdx::bind(f, a, b));
    }
    template <typename F, typename A, typename B, typename C>
    void schedule(F f, A a, B b, C c) {
        schedule(stdx::bind(f, a, b, c));
    }
    template <typename F, typename A, typename B, typename C, typename D>
    void schedule(F f, A a, B b, C c, D d) {
        schedule(stdx::bind(f, a, b, c, d));
    }
    template <typename F, typename A, typename B, typename C, typename D, typename E>
    void schedule(F f, A a, B b, C c, D d, E e) {
        schedule(stdx::bind(f, a, b, c, d, e));
    }

    int tasks_remaining() {
        return _tasksRemaining;
    }

private:
    mongo::mutex _mutex;
    boost::condition _condition;

    std::list<Worker*> _freeWorkers;  // used as LIFO stack (always front)
    std::list<Task> _tasks;           // used as FIFO queue (push_back, pop_front)
    int _tasksRemaining;              // in queue + currently processing
    int _nThreads;  // only used for sanity checking. could be removed in the future.
    const std::string _threadNamePrefix;  // used for logging/diagnostics

    // should only be called by a worker from the worker's thread
    void task_done(Worker* worker);
    friend class Worker;
};

}  // namespace threadpool

using threadpool::ThreadPool;

}  // namespace mongo
