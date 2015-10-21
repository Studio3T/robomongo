/*
 *    Copyright (C) 2010 10gen Inc.
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

#include <string>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/condition.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>

#include "mongo/client/dbclientinterface.h"
#include "mongo/db/jsobj.h"
#include "mongo/platform/atomic_word.h"
#include "mongo/util/timer.h"

namespace pcrecpp {
class RE;
}  // namespace pcrecpp;

namespace mongo {

/**
 * Configuration object describing a bench run activity.
 */
class BenchRunConfig : private boost::noncopyable {
public:
    /**
     * Create a new BenchRunConfig object, and initialize it from the BSON
     * document, "args".
     *
     * Caller owns the returned object, and is responsible for its deletion.
     */
    static BenchRunConfig* createFromBson(const BSONObj& args);

    BenchRunConfig();

    void initializeFromBson(const BSONObj& args);

    // Create a new connection to the mongo instance specified by this configuration.
    DBClientBase* createConnection() const;

    /**
     * Connection std::string describing the host to which to connect.
     */
    std::string host;

    /**
     * Name of the database on which to operate.
     */
    std::string db;

    /**
     * Optional username for authenticating to the database.
     */
    std::string username;

    /**
     * Optional password for authenticating to the database.
     *
     * Only useful if username is non-empty.
     */
    std::string password;

    /**
     * Number of parallel threads to perform the bench run activity.
     */
    unsigned parallel;

    /**
     * Desired duration of the bench run activity, in seconds.
     *
     * NOTE: Only used by the javascript benchRun() and benchRunSync() functions.
     */
    double seconds;

    /// Base random seed for threads
    int64_t randomSeed;

    bool hideResults;
    bool handleErrors;
    bool hideErrors;

    boost::shared_ptr<pcrecpp::RE> trapPattern;
    boost::shared_ptr<pcrecpp::RE> noTrapPattern;
    boost::shared_ptr<pcrecpp::RE> watchPattern;
    boost::shared_ptr<pcrecpp::RE> noWatchPattern;

    /**
     * Operation description.  A BSON array of objects, each describing a single
     * operation.
     *
     * Every thread in a benchRun job will perform these operations in sequence, restarting at
     * the beginning when the end is reached, until the job is stopped.
     *
     * TODO: Document the operation objects.
     *
     * TODO: Introduce support for performing each operation exactly N times.
     */
    BSONObj ops;

    bool throwGLE;
    bool breakOnTrap;

private:
    /// Initialize a config object to its default values.
    void initializeToDefaults();
};

/**
 * An event counter for events that have an associated duration.
 *
 * Not thread safe.  Expected use is one instance per thread during parallel execution.
 */
class BenchRunEventCounter : private boost::noncopyable {
public:
    /// Constructs a zeroed out counter.
    BenchRunEventCounter();
    ~BenchRunEventCounter();

    /**
     * Zero out the counter.
     */
    void reset();

    /**
     * Conceptually the equivalent of "+=".  Adds "other" into this.
     */
    void updateFrom(const BenchRunEventCounter& other);

    /**
     * Count one instance of the event, which took "timeMicros" microseconds.
     */
    void countOne(long long timeMicros) {
        ++_numEvents;
        _totalTimeMicros += timeMicros;
    }

    /**
     * Get the total number of microseconds ellapsed during all observed events.
     */
    unsigned long long getTotalTimeMicros() const {
        return _totalTimeMicros;
    }

    /**
     * Get the number of observed events.
     */
    unsigned long long getNumEvents() const {
        return _numEvents;
    }

private:
    unsigned long long _numEvents;
    long long _totalTimeMicros;
};

/**
 * RAII object for tracing an event.
 *
 * Construct an instance of this at the beginning of an event, and have it go out of scope at
 * the end, to facilitate tracking events.
 *
 * This type can be used to separately count failures and successes by passing two event
 * counters to the BenchRunEventCounter constructor, and calling "succeed()" on the object at
 * the end of a successful event.  If an exception is thrown, the fail counter will receive the
 * event, and otherwise, the succes counter will.
 *
 * In all cases, the counter objects must outlive the trace object.
 */
class BenchRunEventTrace : private boost::noncopyable {
public:
    explicit BenchRunEventTrace(BenchRunEventCounter* eventCounter) {
        initialize(eventCounter, eventCounter, false);
    }

    BenchRunEventTrace(BenchRunEventCounter* successCounter,
                       BenchRunEventCounter* failCounter,
                       bool defaultToFailure = true) {
        initialize(successCounter, failCounter, defaultToFailure);
    }

    ~BenchRunEventTrace() {
        (_succeeded ? _successCounter : _failCounter)->countOne(_timer.micros());
    }

    void succeed() {
        _succeeded = true;
    }
    void fail() {
        _succeeded = false;
    }

private:
    void initialize(BenchRunEventCounter* successCounter,
                    BenchRunEventCounter* failCounter,
                    bool defaultToFailure) {
        _successCounter = successCounter;
        _failCounter = failCounter;
        _succeeded = !defaultToFailure;
    }

    Timer _timer;
    BenchRunEventCounter* _successCounter;
    BenchRunEventCounter* _failCounter;
    bool _succeeded;
};

/**
 * Statistics object representing the result of a bench run activity.
 */
class BenchRunStats : private boost::noncopyable {
public:
    BenchRunStats();
    ~BenchRunStats();

    void reset();

    void updateFrom(const BenchRunStats& other);

    bool error;
    unsigned long long errCount;
    unsigned long long opCount;

    BenchRunEventCounter findOneCounter;
    BenchRunEventCounter updateCounter;
    BenchRunEventCounter insertCounter;
    BenchRunEventCounter deleteCounter;
    BenchRunEventCounter queryCounter;
    BenchRunEventCounter commandCounter;

    std::map<std::string, long long> opcounters;
    std::vector<BSONObj> trappedErrors;
};

/**
 * State of a BenchRun activity.
 *
 * Logically, the states are "starting up", "running" and "finished."
 */
class BenchRunState : private boost::noncopyable {
public:
    enum State { BRS_STARTING_UP, BRS_RUNNING, BRS_FINISHED };

    explicit BenchRunState(unsigned numWorkers);
    ~BenchRunState();

    //
    // Functions called by the job-controlling thread, through an instance of BenchRunner.
    //

    /**
     * Block until the current state is "awaitedState."
     *
     * massert() (uassert()?) if "awaitedState" is unreachable from
     * the current state.
     */
    void waitForState(State awaitedState);

    /**
     * Notify the worker threads to wrap up.  Does not block.
     */
    void tellWorkersToFinish();

    /**
     * Notify the worker threads to collect statistics.  Does not block.
     */
    void tellWorkersToCollectStats();

    /// Check that the current state is BRS_FINISHED.
    void assertFinished();

    //
    // Functions called by the worker threads, through instances of BenchRunWorker.
    //

    /**
     * Predicate that workers call to see if they should finish (as a result of a call
     * to tellWorkersToFinish()).
     */
    bool shouldWorkerFinish();

    /**
    * Predicate that workers call to see if they should start collecting stats (as a result
    * of a call to tellWorkersToCollectStats()).
    */
    bool shouldWorkerCollectStats();

    /**
     * Called by each BenchRunWorker from within its thread context, immediately before it
     * starts sending requests to the configured mongo instance.
     */
    void onWorkerStarted();

    /**
     * Called by each BenchRunWorker from within its thread context, shortly after it finishes
     * sending requests to the configured mongo instance.
     */
    void onWorkerFinished();

private:
    boost::mutex _mutex;
    boost::condition _stateChangeCondition;
    unsigned _numUnstartedWorkers;
    unsigned _numActiveWorkers;
    AtomicUInt32 _isShuttingDown;
    AtomicUInt32 _isCollectingStats;
};

/**
 * A single worker in the bench run activity.
 *
 * Represents the behavior of one thread working in a bench run activity.
 */
class BenchRunWorker : private boost::noncopyable {
public:
    /**
     * Create a new worker, performing one thread's worth of the activity described in
     * "config", and part of the larger activity with state "brState".  Both "config"
     * and "brState" must exist for the life of this object.
     *
     * "id" is a positive integer which should uniquely identify the worker.
     */
    BenchRunWorker(size_t id,
                   const BenchRunConfig* config,
                   BenchRunState* brState,
                   int64_t randomSeed);
    ~BenchRunWorker();

    /**
     * Start performing the "work" behavior in a new thread.
     */
    void start();

    /**
     * Get the run statistics for a worker.
     *
     * Should only be observed _after_ the worker has signaled its completion by calling
     * onWorkerFinished() on the BenchRunState passed into its constructor.
     */
    const BenchRunStats& stats() const {
        return _stats;
    }

private:
    /// The main method of the worker, executed inside the thread launched by start().
    void run();

    /// The function that actually sets about generating the load described in "_config".
    void generateLoadOnConnection(DBClientBase* conn);

    /// Predicate, used to decide whether or not it's time to terminate the worker.
    bool shouldStop() const;
    /// Predicate, used to decide whether or not it's time to collect statistics
    bool shouldCollectStats() const;

    size_t _id;
    const BenchRunConfig* _config;
    BenchRunState* _brState;
    BenchRunStats _stats;
    /// Dummy stats to use before observation period.
    BenchRunStats _statsBlackHole;
    int64_t _randomSeed;
};

/**
 * Object representing a "bench run" activity.
 */
class BenchRunner : private boost::noncopyable {
public:
    /**
     * Utility method to create a new bench runner from a BSONObj representation
     * of a configuration.
     *
     * TODO: This is only really for the use of the javascript benchRun() methods,
     * and should probably move out of the BenchRunner class.
     */
    static BenchRunner* createWithConfig(const BSONObj& configArgs);

    /**
     * Look up a bench runner object by OID.
     *
     * TODO: Same todo as for "createWithConfig".
     */
    static BenchRunner* get(OID oid);

    /**
     * Stop a running "runner", and return a BSON representation of its resultant
     * BenchRunStats.
     *
     * TODO: Same as for "createWithConfig".
     */
    static BSONObj finish(BenchRunner* runner);

    /**
     * Create a new bench runner, to perform the activity described by "*config."
     *
     * Takes ownership of "config", and will delete it.
     */
    explicit BenchRunner(BenchRunConfig* config);
    ~BenchRunner();

    /**
     * Start the activity.  Only call once per instance of BenchRunner.
     */
    void start();

    /**
     * Stop the activity.  Block until the activitiy has stopped.
     */
    void stop();

    /**
     * Store the collected event data from a completed bench run activity into "stats."
     *
     * Illegal to call until after stop() returns.
     */
    void populateStats(BenchRunStats* stats);

    OID oid() const {
        return _oid;
    }

    const BenchRunConfig& config() const {
        return *_config;
    }  // TODO: Remove this function.

    // JS bindings
    static BSONObj benchFinish(const BSONObj& argsFake, void* data);
    static BSONObj benchStart(const BSONObj& argsFake, void* data);
    static BSONObj benchRunSync(const BSONObj& argsFake, void* data);

private:
    // TODO: Same as for createWithConfig.
    static boost::mutex _staticMutex;
    static std::map<OID, BenchRunner*> _activeRuns;

    OID _oid;
    BenchRunState _brState;
    Timer* _brTimer;
    unsigned long long _microsElapsed;
    boost::scoped_ptr<BenchRunConfig> _config;
    std::vector<BenchRunWorker*> _workers;
};

}  // namespace mongo
