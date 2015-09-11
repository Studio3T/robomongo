// distlock.h

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

#include "mongo/platform/basic.h"
#include "mongo/client/connpool.h"
#include "mongo/client/export_macros.h"
#include "mongo/client/syncclusterconnection.h"
#include "mongo/logger/labeled_level.h"


namespace mongo {

namespace {

enum TimeConstants {
    LOCK_TIMEOUT = 15 * 60 * 1000,
    LOCK_SKEW_FACTOR = 30,
    LOCK_PING = LOCK_TIMEOUT / LOCK_SKEW_FACTOR,
    MAX_LOCK_NET_SKEW = LOCK_TIMEOUT / LOCK_SKEW_FACTOR,
    MAX_LOCK_CLOCK_SKEW = LOCK_TIMEOUT / LOCK_SKEW_FACTOR,
    NUM_LOCK_SKEW_CHECKS = 3,
};

// The maximum clock skew we need to handle between config servers is
// 2 * MAX_LOCK_NET_SKEW + MAX_LOCK_CLOCK_SKEW.

// Net effect of *this* clock being slow is effectively a multiplier on the max net skew
// and a linear increase or decrease of the max clock skew.
}

/**
 * Exception class to encapsulate exceptions while managing distributed locks
 */
class MONGO_CLIENT_API LockException : public DBException {
public:
    LockException(const char* msg, int code) : DBException(msg, code) {}
    LockException(const std::string& msg, int code) : DBException(msg, code) {}
    virtual ~LockException() throw() {}
};

/**
 * Indicates an error in retrieving time values from remote servers.
 */
class MONGO_CLIENT_API TimeNotFoundException : public LockException {
public:
    TimeNotFoundException(const char* msg, int code) : LockException(msg, code) {}
    TimeNotFoundException(const std::string& msg, int code) : LockException(msg, code) {}
    virtual ~TimeNotFoundException() throw() {}
};

/**
 * The distributed lock is a configdb backed way of synchronizing system-wide tasks. A task
 * must be identified by a unique name across the system (e.g., "balancer"). A lock is taken
 * by writing a document in the configdb's locks collection with that name.
 *
 * To be maintained, each taken lock needs to be revalidated ("pinged") within a
 * pre-established amount of time. This class does this maintenance automatically once a
 * DistributedLock object was constructed. The ping procedure records the local time to
 * the ping document, but that time is untrusted and is only used as a point of reference
 * of whether the ping was refreshed or not. Ultimately, the clock a configdb is the source
 * of truth when determining whether a ping is still fresh or not. This is achieved by
 * (1) remembering the ping document time along with config server time when unable to
 * take a lock, and (2) ensuring all config servers report similar times and have similar
 * time rates (the difference in times must start and stay small).
 *
 * Lock states include:
 * 0: unlocked
 * 1: about to be locked
 * 2: locked
 *
 * Valid state transitions:
 * 0 -> 1
 * 1 -> 2
 * 2 -> 0
 *
 * Note that at any point in time, a lock can be force unlocked if the ping for the lock
 * becomes too stale.
 */
class MONGO_CLIENT_API DistributedLock {
public:
    static logger::LabeledLevel logLvl;

    struct PingData {
        PingData(const std::string& _id, Date_t _lastPing, Date_t _remote, OID _ts)
            : id(_id), lastPing(_lastPing), remote(_remote), ts(_ts) {}

        PingData() : id(""), lastPing(0), remote(0), ts() {}

        std::string id;
        Date_t lastPing;
        Date_t remote;
        OID ts;
    };

    class LastPings {
    public:
        LastPings() : _mutex("DistributedLock::LastPings") {}
        ~LastPings() {}

        PingData getLastPing(const ConnectionString& conn, const std::string& lockName);
        void setLastPing(const ConnectionString& conn,
                         const std::string& lockName,
                         const PingData& pd);

        mongo::mutex _mutex;
        std::map<std::pair<std::string, std::string>, PingData> _lastPings;
    };

    static LastPings lastPings;

    /**
     * The constructor does not connect to the configdb yet and constructing does not mean the lock
     * was acquired. Construction does trigger a lock "pinging" mechanism, though.
     *
     * @param conn address of config(s) server(s)
     * @param name identifier for the lock
     * @param lockTimeout how long can the log go "unpinged" before a new attempt to lock steals it
     *      (in minutes).
     * @param lockPing how long to wait between lock pings
     * @param legacy use legacy logic
     *
     */
    DistributedLock(const ConnectionString& conn,
                    const std::string& name,
                    unsigned long long lockTimeout = 0,
                    bool asProcess = false);
    ~DistributedLock(){};

    /**
     * Attempts to acquire 'this' lock, checking if it could or should be stolen from the previous
     * holder. Please consider using the dist_lock_try construct to acquire this lock in an
     * exception safe way.
     *
     * @param why human readable description of why the lock is being taken (used to log)
     * @param whether this is a lock re-entry or a new lock
     * @param other configdb's lock document that is currently holding the lock, if lock is taken,
     * or our own lock details if not
     * @return true if it managed to grab the lock
     */
    bool lock_try(const std::string& why,
                  bool reenter = false,
                  BSONObj* other = 0,
                  double timeout = 0.0);

    /**
     * Returns OK if this lock is held (but does not guarantee that this owns it) and
     * it was possible to confirm that, within 'timeout' seconds, if provided, with the
     * config servers.
     */
    Status checkStatus(double timeout);

    /**
     * Releases a previously taken lock.
     */
    void unlock(BSONObj* oldLockPtr = NULL);

    Date_t getRemoteTime();

    bool isRemoteTimeSkewed();

    const std::string& getProcessId();

    const ConnectionString& getRemoteConnection();

    /**
     * Checks the skew among a cluster of servers and returns true if the min and max clock
     * times among the servers are within maxClockSkew.
     */
    static bool checkSkew(const ConnectionString& cluster,
                          unsigned skewChecks = NUM_LOCK_SKEW_CHECKS,
                          unsigned long long maxClockSkew = MAX_LOCK_CLOCK_SKEW,
                          unsigned long long maxNetSkew = MAX_LOCK_NET_SKEW);

    /**
     * Get the remote time from a server or cluster
     */
    static Date_t remoteTime(const ConnectionString& cluster,
                             unsigned long long maxNetSkew = MAX_LOCK_NET_SKEW);

    static bool killPinger(DistributedLock& lock);

    /**
     * Namespace for lock pings
     */
    static const std::string lockPingNS;

    /**
     * Namespace for locks
     */
    static const std::string locksNS;

    const ConnectionString _conn;
    const std::string _name;
    const std::string _processId;

    // Timeout for lock, usually LOCK_TIMEOUT
    const unsigned long long _lockTimeout;
    const unsigned long long _maxClockSkew;
    const unsigned long long _maxNetSkew;
    const unsigned long long _lockPing;

private:
    void resetLastPing() {
        lastPings.setLastPing(_conn, _name, PingData());
    }
    void setLastPing(const PingData& pd) {
        lastPings.setLastPing(_conn, _name, pd);
    }
    PingData getLastPing() {
        return lastPings.getLastPing(_conn, _name);
    }

    // May or may not exist, depending on startup
    mongo::mutex _mutex;
    std::string _threadId;
};

// Helper functions for tests, allows us to turn the creation of a lock pinger on and off.
// *NOT* thread-safe
bool MONGO_CLIENT_API isLockPingerEnabled();
void MONGO_CLIENT_API setLockPingerEnabled(bool enabled);


class MONGO_CLIENT_API dist_lock_try {
public:
    dist_lock_try() : _lock(NULL), _got(false) {}

    dist_lock_try(const dist_lock_try& that)
        : _lock(that._lock), _got(that._got), _other(that._other) {
        _other.getOwned();

        // Make sure the lock ownership passes to this object,
        // so we only unlock once.
        ((dist_lock_try&)that)._got = false;
        ((dist_lock_try&)that)._lock = NULL;
        ((dist_lock_try&)that)._other = BSONObj();
    }

    // Needed so we can handle lock exceptions in context of lock try.
    dist_lock_try& operator=(const dist_lock_try& that) {
        if (this == &that)
            return *this;

        _lock = that._lock;
        _got = that._got;
        _other = that._other;
        _other.getOwned();
        _why = that._why;

        // Make sure the lock ownership passes to this object,
        // so we only unlock once.
        ((dist_lock_try&)that)._got = false;
        ((dist_lock_try&)that)._lock = NULL;
        ((dist_lock_try&)that)._other = BSONObj();

        return *this;
    }

    dist_lock_try(DistributedLock* lock, const std::string& why, double timeout = 0.0)
        : _lock(lock), _why(why) {
        _got = _lock->lock_try(why, false, &_other, timeout);
    }

    ~dist_lock_try() {
        if (_got) {
            verify(!_other.isEmpty());
            _lock->unlock(&_other);
        }
    }

    /**
     * Returns not OK  if the lock is known _not_ to be held.
     */
    Status checkStatus(double timeout) {
        if (!_lock) {
            return Status(ErrorCodes::LockFailed, "Lock is not currently set up");
        }

        if (!_got) {
            return Status(ErrorCodes::LockFailed,
                          str::stream() << "Lock " << _lock->_name << " is currently held by "
                                        << _other);
        }

        return _lock->checkStatus(timeout);
    }

    bool got() const {
        return _got;
    }
    BSONObj other() const {
        return _other;
    }

private:
    DistributedLock* _lock;
    bool _got;
    BSONObj _other;
    std::string _why;
};

/**
 * Scoped wrapper for a distributed lock acquisition attempt.  One or more attempts to acquire
 * the distributed lock are managed by this class, and the distributed lock is unlocked if
 * successfully acquired on object destruction.
 */
class MONGO_CLIENT_API ScopedDistributedLock {
public:
    static const long long kDefaultLockTryIntervalMillis;
    static const long long kDefaultSocketTimeoutMillis;

    ScopedDistributedLock(const ConnectionString& conn, const std::string& name);

    ~ScopedDistributedLock();

    /**
     * Tries to obtain the lock once.
     *
     * Returns OK if the lock was successfully acquired.
     * Returns ErrorCodes::DistributedClockSkewed when a clock skew is detected.
     * Returns ErrorCodes::LockBusy if the lock is being held.
     */
    Status tryAcquire();

    /**
     * Tries to unlock the lock if acquired.  Cannot report an error or block indefinitely
     * (though it may log messages or continue retrying in a non-blocking way).
     */
    void unlock();

    /**
     * Tries multiple times to unlock the lock, using the specified lock try interval, until
     * a certain amount of time has passed.
     *
     * waitForMillis = 0 indicates there should only be one attempt to acquire the lock, and
     * no waiting.
     * waitForMillis = -1 indicates we should retry indefinitely.
     *
     * Returns OK if the lock was successfully acquired.
     * Returns ErrorCodes::DistributedClockSkewed when a clock skew is detected.
     * Returns ErrorCodes::LockBusy if the lock is being held.
     */
    Status acquire(long long waitForMillis);

    /**
     * If lock is held, remotely verifies that the lock has not been forced as a sanity check.
     * If the lock is not held or cannot be verified, returns not OK.
     */
    Status checkStatus();

    bool isAcquired() const {
        return _acquired;
    }

    ConnectionString getConfigConnectionString() const {
        return _lock._conn;
    }

    void setLockTryIntervalMillis(long long lockTryIntervalMillis) {
        _lockTryIntervalMillis = lockTryIntervalMillis;
    }

    long long getLockTryIntervalMillis() const {
        return _lockTryIntervalMillis;
    }

    void setLockMessage(const std::string& why) {
        _why = why;
    }

    std::string getLockMessage() const {
        return _why;
    }

    void setSocketTimeoutMillis(long long socketTimeoutMillis) {
        _socketTimeoutMillis = socketTimeoutMillis;
    }

    long long getSocketTimeoutMillis() const {
        return _socketTimeoutMillis;
    }

private:
    DistributedLock _lock;
    std::string _why;
    long long _lockTryIntervalMillis;
    long long _socketTimeoutMillis;

    bool _acquired;
    BSONObj _other;
};
}
