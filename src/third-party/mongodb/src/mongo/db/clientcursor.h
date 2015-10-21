/**
 *    Copyright (C) 2008 10gen Inc.
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
#include <boost/scoped_ptr.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "mongo/db/jsobj.h"
#include "mongo/db/keypattern.h"
#include "mongo/db/query/plan_executor.h"
#include "mongo/db/record_id.h"
#include "mongo/s/collection_metadata.h"
#include "mongo/util/background.h"
#include "mongo/util/net/message.h"

namespace mongo {

typedef boost::recursive_mutex::scoped_lock recursive_scoped_lock;
class ClientCursor;
class Collection;
class CurOp;
class CursorManager;
class Database;
class NamespaceDetails;
class ParsedQuery;
class RecoveryUnit;

typedef long long CursorId; /* passed to the client so it can send back on getMore */
static const CursorId INVALID_CURSOR_ID = -1;  // But see SERVER-5726.

/**
 * ClientCursor is a wrapper that represents a cursorid from our database application's
 * perspective.
 */
class ClientCursor : private boost::noncopyable {
public:
    /**
     * This ClientCursor constructor creates a cursorid that can be used with getMore and
     * killCursors.  "cursorManager" is the object that will manage the lifetime of this
     * cursor, and "ns" is the namespace string that should be associated with this cursor (e.g.
     * "test.foo", "test.$cmd.listCollections", etc).
     */
    ClientCursor(CursorManager* cursorManager,
                 PlanExecutor* exec,
                 const std::string& ns,
                 int qopts = 0,
                 const BSONObj query = BSONObj(),
                 bool isAggCursor = false);

    /**
     * This ClientCursor is used to track sharding state for the given collection.
     */
    explicit ClientCursor(const Collection* collection);

    //
    // Basic accessors
    //

    CursorId cursorid() const {
        return _cursorid;
    }
    std::string ns() const {
        return _ns;
    }
    CursorManager* cursorManager() const {
        return _cursorManager;
    }
    bool isAggCursor() const {
        return _isAggCursor;
    }

    //
    // Pinning functionality.
    //

    /**
     * Marks this ClientCursor as in use.  unsetPinned() must be called before the destructor of
     * this ClientCursor is invoked.
     */
    void setPinned() {
        _isPinned = true;
    }

    /**
     * Marks this ClientCursor as no longer in use.
     */
    void unsetPinned() {
        _isPinned = false;
    }

    bool isPinned() const {
        return _isPinned;
    }

    /**
     * This is called when someone is dropping a collection or something else that
     * goes through killing cursors.
     * It removes the responsiilibty of de-registering from ClientCursor.
     * Responsibility for deleting the ClientCursor doesn't change from this call
     * see PlanExecutor::kill.
     */
    void kill();

    //
    // Timing and timeouts
    //

    /**
     * @param millis amount of idle passed time since last call
     * note called outside of locks (other than ccmutex) so care must be exercised
     */
    bool shouldTimeout(int millis);
    void setIdleTime(int millis);
    int idleTime() const {
        return _idleAgeMillis;
    }

    uint64_t getLeftoverMaxTimeMicros() const {
        return _leftoverMaxTimeMicros;
    }
    void setLeftoverMaxTimeMicros(uint64_t leftoverMaxTimeMicros) {
        _leftoverMaxTimeMicros = leftoverMaxTimeMicros;
    }

    //
    // Sharding-specific data.  TODO: Document.
    //

    void setCollMetadata(CollectionMetadataPtr metadata) {
        _collMetadata = metadata;
    }
    CollectionMetadataPtr getCollMetadata() {
        return _collMetadata;
    }

    //
    // Replication-related stuff.  TODO: Document and clean.
    //

    void updateSlaveLocation(OperationContext* txn, CurOp& curop);
    void slaveReadTill(const OpTime& t) {
        _slaveReadTill = t;
    }
    /** Just for testing. */
    OpTime getSlaveReadTill() const {
        return _slaveReadTill;
    }

    //
    // Query-specific functionality that may be adapted for the PlanExecutor.
    //

    PlanExecutor* getExecutor() const {
        return _exec.get();
    }
    int queryOptions() const {
        return _queryOptions;
    }
    const BSONObj& getQuery() const {
        return _query;
    }

    // Used by ops/query.cpp to stash how many results have been returned by a query.
    int pos() const {
        return _pos;
    }
    void incPos(int n) {
        _pos += n;
    }
    void setPos(int n) {
        _pos = n;
    }

    static long long totalOpen();

    //
    // Storage engine state for getMore.
    //

    bool hasRecoveryUnit() const {
        return _ownedRU.get() || _unownedRU;
    }

    /**
     *
     * If a ClientCursor is created via DBDirectClient, it uses the same storage engine
     * context as the DBDirectClient caller.  We store this context in _unownedRU.  We use
     * this to verify that all further callers use the same RecoveryUnit.
     *
     * Once a ClientCursor has an unowned RecoveryUnit, it will always have one.
     *
     * Sets the unowned RecoveryUnit to 'ru'.  Does NOT take ownership of the pointer.
     */
    void setUnownedRecoveryUnit(RecoveryUnit* ru);

    /**
     * Return the unowned RecoveryUnit.  'this' does not own pointer and therefore cannot
     * transfer ownership.
     */
    RecoveryUnit* getUnownedRecoveryUnit() const;

    /**
     * If a ClientCursor is created via a client request, we bind its lifetime to the
     * ClientCursor's by storing it un _ownedRU.  In order to execute the query over repeated
     * network requests, we have to keep the execution state around.
     */

    /**
     * Set the owned recovery unit to 'ru'.  Takes ownership of it.  If there is a previous
     * owned recovery unit, it is deleted.
     */
    void setOwnedRecoveryUnit(RecoveryUnit* ru);

    /**
     * Returns the owned recovery unit.  Ownership is transferred to the caller.
     */
    RecoveryUnit* releaseOwnedRecoveryUnit();

private:
    friend class CursorManager;
    friend class ClientCursorPin;

    /**
     * Only friends are allowed to destroy ClientCursor objects.
     */
    ~ClientCursor();

    /**
     * Initialization common between both constructors for the ClientCursor. The database must
     * be stable when this is called, because cursors hang off the collection.
     */
    void init();

    //
    // ClientCursor-specific data, independent of the underlying execution type.
    //

    // The ID of the ClientCursor.
    CursorId _cursorid;

    // The namespace we're operating on.
    std::string _ns;

    CursorManager* _cursorManager;

    // if we've added it to the total open counter yet
    bool _countedYet;

    // How many objects have been returned by the find() so far?
    int _pos;

    // If this cursor was created by a find operation, '_query' holds the query predicate for
    // the find. If this cursor was created by a command (e.g. the aggregate command), then
    // '_query' holds the command specification received from the client.
    BSONObj _query;

    // See the QueryOptions enum in dbclient.h
    int _queryOptions;

    // Is this ClientCursor backed by an aggregation pipeline?  Defaults to false.
    //
    // Agg executors differ from others in that they manage their own locking internally and
    // should not be killed or destroyed when the underlying collection is deleted.
    //
    // Note: This should *not* be set for the internal cursor used as input to an aggregation.
    bool _isAggCursor;

    // Is this cursor in use?  Defaults to false.
    bool _isPinned;

    // Is the "no timeout" flag set on this cursor?  If false, this cursor may be targeted for
    // deletion after an interval of inactivity.  Defaults to false.
    bool _isNoTimeout;

    // TODO: document better.
    OpTime _slaveReadTill;

    // How long has the cursor been idle?
    int _idleAgeMillis;

    // TODO: Document.
    uint64_t _leftoverMaxTimeMicros;

    // For chunks that are being migrated, there is a period of time when that chunks data is in
    // two shards, the donor and the receiver one. That data is picked up by a cursor on the
    // receiver side, even before the migration was decided.  The CollectionMetadata allow one
    // to inquiry if any given document of the collection belongs indeed to this shard or if it
    // is coming from (or a vestige of) an ongoing migration.
    CollectionMetadataPtr _collMetadata;

    // Only one of these is not-NULL.
    RecoveryUnit* _unownedRU;
    std::auto_ptr<RecoveryUnit> _ownedRU;
    // NOTE: _ownedRU must come before _exec, because _ownedRU must outlive _exec.
    // The storage engine can have resources in the PlanExecutor that rely on
    // the RecoveryUnit being alive.

    //
    // The underlying execution machinery.
    //
    boost::scoped_ptr<PlanExecutor> _exec;
};

/**
 * ClientCursorPin is an RAII class that manages the pinned state of a ClientCursor.
 * ClientCursorPin objects pin the given cursor upon construction, and release the pin upon
 * destruction.
 *
 * A pin extends the lifetime of a ClientCursor object until the pin's release.  Pinned
 * ClientCursor objects cannot not be killed due to inactivity, and cannot be killed by user
 * kill requests.  When a CursorManager is destroyed (e.g. by a collection drop), ownership of
 * any still-pinned ClientCursor objects is transferred to their managing ClientCursorPin
 * objects.
 *
 * Example usage:
 * {
 *     ClientCursorPin pin(cursorManager, cursorid);
 *     ClientCursor* cursor = pin.c();
 *     if (cursor) {
 *         // Use cursor.
 *     }
 *     // Pin automatically released on block exit.
 * }
 *
 * Clients that wish to access ClientCursor objects owned by collection cursor managers must
 * hold the collection lock during pin acquisition and pin release.  This guards from a
 * collection drop (which requires an exclusive lock on the collection) occurring concurrently
 * with the pin request or unpin request.
 *
 * Clients that wish to access ClientCursor objects owned by the global cursor manager need not
 * hold any locks; the global cursor manager can only be destroyed by a process exit.
 */
class ClientCursorPin {
    MONGO_DISALLOW_COPYING(ClientCursorPin);

public:
    /**
     * Asks "cursorManager" to set a pin on the ClientCursor associated with "cursorid".  If no
     * such cursor exists, does nothing.  If the cursor is already pinned, throws a
     * UserException.
     */
    ClientCursorPin(CursorManager* cursorManager, long long cursorid);

    /**
     * Calls release().
     */
    ~ClientCursorPin();

    /**
     * Releases the pin.  It does not delete the underlying cursor unless ownership has passed
     * to us after kill.  Turns into a no-op if release() or deleteUnderlying() have already
     * been called on this pin.
     */
    void release();

    /**
     * Deletes the underlying cursor.  Cannot be called if release() or deleteUnderlying() have
     * already been called on this pin.
     */
    void deleteUnderlying();

    ClientCursor* c() const;

private:
    ClientCursor* _cursor;
};

void startClientCursorMonitor();

}  // namespace mongo
