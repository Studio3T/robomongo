// @file curop.h

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
 *    must comply with the GNU Affero General Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */


#pragma once

#include <boost/noncopyable.hpp>

#include "mongo/db/client.h"
#include "mongo/db/server_options.h"
#include "mongo/platform/atomic_word.h"
#include "mongo/util/concurrency/spin_lock.h"
#include "mongo/util/net/hostandport.h"
#include "mongo/util/progress_meter.h"
#include "mongo/util/thread_safe_string.h"
#include "mongo/util/time_support.h"


namespace mongo {

class Client;
class Command;
class CurOp;

/**
 * stores a copy of a bson obj in a fixed size buffer
 * if its too big for the buffer, says "too big"
 * useful for keeping a copy around indefinitely without wasting a lot of space or doing malloc
 */
class CachedBSONObjBase {
public:
    static BSONObj _tooBig;  // { $msg : "query not recording (too large)" }
};

template <size_t BUFFER_SIZE>
class CachedBSONObj : public CachedBSONObjBase {
public:
    enum { TOO_BIG_SENTINEL = 1 };

    CachedBSONObj() {
        _size = (int*)_buf;
        reset();
    }

    void reset(int sz = 0) {
        _lock.lock();
        _reset(sz);
        _lock.unlock();
    }

    void set(const BSONObj& o) {
        scoped_spinlock lk(_lock);
        size_t sz = o.objsize();
        if (sz > sizeof(_buf)) {
            _reset(TOO_BIG_SENTINEL);
        } else {
            memcpy(_buf, o.objdata(), sz);
        }
    }

    int size() const {
        return *_size;
    }
    bool have() const {
        return size() > 0;
    }
    bool tooBig() const {
        return size() == TOO_BIG_SENTINEL;
    }

    BSONObj get() const {
        scoped_spinlock lk(_lock);
        return _get();
    }

    void append(BSONObjBuilder& b, const StringData& name) const {
        scoped_spinlock lk(_lock);
        BSONObj temp = _get();
        b.append(name, temp);
    }

private:
    /** you have to be locked when you call this */
    BSONObj _get() const {
        int sz = size();
        if (sz == 0)
            return BSONObj();
        if (sz == TOO_BIG_SENTINEL)
            return _tooBig;
        return BSONObj(_buf).copy();
    }

    /** you have to be locked when you call this */
    void _reset(int sz) {
        _size[0] = sz;
    }

    mutable SpinLock _lock;
    int* _size;
    char _buf[BUFFER_SIZE];
};

/* lifespan is different than CurOp because of recursives with DBDirectClient */
class OpDebug {
public:
    OpDebug() : planSummary(2048) {
        reset();
    }

    void reset();

    void recordStats();

    std::string report(const CurOp& curop, const SingleThreadedLockStats& lockStats) const;

    /**
     * Appends information about the current operation to "builder"
     *
     * @param curop reference to the CurOp that owns this OpDebug
     * @param lockStats lockStats object containing locking information about the operation
     */
    void append(const CurOp& curop,
                const SingleThreadedLockStats& lockStats,
                BSONObjBuilder& builder) const;

    // -------------------

    StringBuilder extra;  // weird things we need to fix later

    // basic options
    int op;
    bool iscommand;
    ThreadSafeString ns;
    BSONObj query;
    BSONObj updateobj;

    // detailed options
    long long cursorid;
    int ntoreturn;
    int ntoskip;
    bool exhaust;

    // debugging/profile info
    long long nscanned;
    long long nscannedObjects;
    bool idhack;  // indicates short circuited code path on an update to make the update faster
    bool scanAndOrder;    // scanandorder query plan aspect was used
    long long nMatched;   // number of records that match the query
    long long nModified;  // number of records written (no no-ops)
    long long nmoved;     // updates resulted in a move (moves are expensive)
    long long ninserted;
    long long ndeleted;
    bool fastmod;
    bool fastmodinsert;  // upsert of an $operation. builds a default object
    bool upsert;         // true if the update actually did an insert
    int keyUpdates;
    long long writeConflicts;
    ThreadSafeString planSummary;  // a brief std::string describing the query solution

    // New Query Framework debugging/profiling info
    // TODO: should this really be an opaque BSONObj?  Not sure.
    CachedBSONObj<4096> execStats;

    // error handling
    ExceptionInfo exceptionInfo;

    // response info
    int executionTime;
    int nreturned;
    int responseLength;
};

/* Current operation (for the current Client).
   an embedded member of Client class, and typically used from within the mutex there.
*/
class CurOp : boost::noncopyable {
public:
    CurOp(Client* client, CurOp* wrapped = 0);
    ~CurOp();

    bool haveQuery() const {
        return _query.have();
    }
    BSONObj query() const {
        return _query.get();
    }
    void appendQuery(BSONObjBuilder& b, const StringData& name) const {
        _query.append(b, name);
    }

    void enter(const char* ns, int dbProfileLevel);
    void reset();
    void reset(const HostAndPort& remote, int op);
    void markCommand() {
        _isCommand = true;
    }
    OpDebug& debug() {
        return _debug;
    }
    std::string getNS() const {
        return _ns.toString();
    }

    bool shouldDBProfile(int ms) const {
        if (_dbprofile <= 0)
            return false;

        return _dbprofile >= 2 || ms >= serverGlobalParams.slowMS;
    }

    unsigned int opNum() const {
        return _opNum;
    }

    /** if this op is running */
    bool active() const {
        return _active;
    }

    int getOp() const {
        return _op;
    }

    //
    // Methods for controlling CurOp "max time".
    //

    /**
     * Sets the amount of time operation this should be allowed to run, units of microseconds.
     * The special value 0 is "allow to run indefinitely".
     */
    void setMaxTimeMicros(uint64_t maxTimeMicros);

    /**
     * Checks whether this operation has been running longer than its time limit.  Returns
     * false if not, or if the operation has no time limit.
     *
     * Note that KillCurrentOp objects are responsible for interrupting CurOp objects that
     * have exceeded their allotted time; CurOp objects do not interrupt themselves.
     */
    bool maxTimeHasExpired();

    /**
     * Returns the number of microseconds remaining for this operation's time limit, or the
     * special value 0 if the operation has no time limit.
     *
     * Calling this method is more expensive than calling its sibling "maxTimeHasExpired()",
     * since an accurate measure of remaining time needs to be calculated.
     */
    uint64_t getRemainingMaxTimeMicros() const;

    //
    // Methods for getting/setting elapsed time.
    //

    void ensureStarted();
    bool isStarted() const {
        return _start > 0;
    }
    long long startTime() {  // micros
        ensureStarted();
        return _start;
    }
    void done() {
        _active = false;
        _end = curTimeMicros64();
    }

    long long totalTimeMicros() {
        massert(12601, "CurOp not marked done yet", !_active);
        return _end - startTime();
    }
    int totalTimeMillis() {
        return (int)(totalTimeMicros() / 1000);
    }
    long long elapsedMicros() {
        return curTimeMicros64() - startTime();
    }
    int elapsedMillis() {
        return (int)(elapsedMicros() / 1000);
    }
    int elapsedSeconds() {
        return elapsedMillis() / 1000;
    }

    void setQuery(const BSONObj& query) {
        _query.set(query);
    }
    Client* getClient() const {
        return _client;
    }

    Command* getCommand() const {
        return _command;
    }
    void setCommand(Command* command) {
        _command = command;
    }

    void reportState(BSONObjBuilder* builder);

    // Fetches less information than "info()"; used to search for ops with certain criteria
    BSONObj description();

    std::string getRemoteString(bool includePort = true) {
        if (includePort)
            return _remote.toString();
        return _remote.host();
    }

    ProgressMeter& setMessage(const char* msg,
                              std::string name = "Progress",
                              unsigned long long progressMeterTotal = 0,
                              int secondsBetween = 3);
    std::string getMessage() const {
        return _message.toString();
    }
    ProgressMeter& getProgressMeter() {
        return _progressMeter;
    }
    CurOp* parent() const {
        return _wrapped;
    }
    void kill();
    bool killPendingStrict() const {
        return _killPending.load();
    }
    bool killPending() const {
        return _killPending.loadRelaxed();
    }
    void yielded() {
        _numYields++;
    }
    int numYields() const {
        return _numYields;
    }

    long long getExpectedLatencyMs() const {
        return _expectedLatencyMs;
    }
    void setExpectedLatencyMs(long long latency) {
        _expectedLatencyMs = latency;
    }

    void recordGlobalTime(bool isWriteLocked, long long micros) const;

    /**
     * this should be used very sparingly
     * generally the Context should set this up
     * but sometimes you want to do it ahead of time
     */
    void setNS(const StringData& ns);

private:
    friend class Client;
    void _reset();

    static AtomicUInt32 _nextOpNum;
    Client* _client;
    CurOp* _wrapped;
    Command* _command;
    long long _start;
    long long _end;
    bool _active;
    int _op;
    bool _isCommand;
    int _dbprofile;  // 0=off, 1=slow, 2=all
    unsigned int _opNum;
    ThreadSafeString _ns;
    HostAndPort _remote;        // CAREFUL here with thread safety
    CachedBSONObj<512> _query;  // CachedBSONObj is thread safe
    OpDebug _debug;
    ThreadSafeString _message;
    ProgressMeter _progressMeter;
    AtomicInt32 _killPending;
    int _numYields;

    // this is how much "extra" time a query might take
    // a writebacklisten for example will block for 30s
    // so this should be 30000 in that case
    long long _expectedLatencyMs;

    // Time limit for this operation.  0 if the operation has no time limit.
    uint64_t _maxTimeMicros;

    /** Nested class that implements tracking of a time limit for a CurOp object. */
    class MaxTimeTracker {
        MONGO_DISALLOW_COPYING(MaxTimeTracker);

    public:
        /** Newly-constructed MaxTimeTracker objects have the time limit disabled. */
        MaxTimeTracker();

        /** Disables the time tracker. */
        void reset();

        /** Returns whether or not time tracking is enabled. */
        bool isEnabled() const {
            return _enabled;
        }

        /**
         * Enables time tracking.  The time limit is set to be "durationMicros" microseconds
         * from "startEpochMicros" (units of microseconds since the epoch).
         *
         * "durationMicros" must be nonzero.
         */
        void setTimeLimit(uint64_t startEpochMicros, uint64_t durationMicros);

        /**
         * Checks whether the time limit has been hit.  Returns false if not, or if time
         * tracking is disabled.
         */
        bool checkTimeLimit();

        /**
         * Returns the number of microseconds remaining for the time limit, or the special
         * value 0 if time tracking is disabled.
         *
         * Calling this method is more expensive than calling its sibling "checkInterval()",
         * since an accurate measure of remaining time needs to be calculated.
         */
        uint64_t getRemainingMicros() const;

    private:
        // Whether or not time tracking is enabled for this operation.
        bool _enabled;

        // Point in time at which the time limit is hit.  Units of microseconds since the
        // epoch.
        uint64_t _targetEpochMicros;

        // Approximate point in time at which the time limit is hit.   Units of milliseconds
        // since the server process was started.
        int64_t _approxTargetServerMillis;
    } _maxTimeTracker;
};
}
