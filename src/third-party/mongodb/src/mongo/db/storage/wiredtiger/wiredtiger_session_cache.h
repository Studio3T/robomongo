// wiredtiger_session_cache.h

/**
 *    Copyright (C) 2014 MongoDB Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
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

#include <list>
#include <string>

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <wiredtiger.h>

#include "mongo/platform/atomic_word.h"
#include "mongo/util/concurrency/spin_lock.h"

namespace mongo {

class WiredTigerKVEngine;

class WiredTigerCachedCursor {
public:
    WiredTigerCachedCursor(uint64_t id, uint64_t gen, WT_CURSOR* cursor)
        : _id(id), _gen(gen), _cursor(cursor) {}

    uint64_t _id;   // Source ID, assigned to each URI
    uint64_t _gen;  // Generation, used to age out old cursors
    WT_CURSOR* _cursor;
};

/**
 * This is a structure that caches 1 cursor for each uri.
 * The idea is that there is a pool of these somewhere.
 * NOT THREADSAFE
 */
class WiredTigerSession {
public:
    /**
     * Creates a new WT session on the specified connection.
     *
     * @param conn WT connection
     * @param cachePartition If the session comes from the session cache, this indicates to
     *          which partition it should be returned. Value of -1 means it doesn't come from
     *          cache and that it should not be cached, but closed directly.
     * @param epoch In which session cache cleanup epoch was this session instantiated. Value
     *          of -1 means that this value is not necessary since the session will not be
     *          cached.
     */
    WiredTigerSession(WT_CONNECTION* conn, int epoch = -1);
    ~WiredTigerSession();

    WT_SESSION* getSession() const {
        return _session;
    }

    WT_CURSOR* getCursor(const std::string& uri, uint64_t id, bool forRecordStore);

    void releaseCursor(uint64_t id, WT_CURSOR* cursor);

    void closeAllCursors();

    int cursorsOut() const {
        return _cursorsOut;
    }

    static uint64_t genCursorId();

    /**
     * For "metadata:" cursors. Guaranteed never to collide with genCursorId() ids.
     */
    static const uint64_t kMetadataCursorId = 0;

private:
    friend class WiredTigerSessionCache;

    // The cursor cache is a list of pairs that contain an ID and cursor
    typedef std::list<WiredTigerCachedCursor> CursorCache;

    // Used internally by WiredTigerSessionCache
    uint64_t _getEpoch() const {
        return _epoch;
    }

    const uint64_t _epoch;
    WT_SESSION* _session;  // owned
    CursorCache _cursors;  // owned
    uint64_t _cursorGen;
    int _cursorsCached, _cursorsOut;
};

class WiredTigerSessionCache {
public:
    WiredTigerSessionCache(WiredTigerKVEngine* engine);
    WiredTigerSessionCache(WT_CONNECTION* conn);
    ~WiredTigerSessionCache();

    WiredTigerSession* getSession();
    void releaseSession(WiredTigerSession* session);

    void closeAll();

    void shuttingDown();

    WT_CONNECTION* conn() const {
        return _conn;
    }

private:
    WiredTigerKVEngine* _engine;  // not owned, might be NULL
    WT_CONNECTION* _conn;         // not owned

    // Regular operations take it in shared mode. Shutdown sets the _shuttingDown flag and
    // then takes it in exclusive mode. This ensures that all threads, which would return
    // sessions to the cache would leak them.
    boost::shared_mutex _shutdownLock;
    AtomicUInt32 _shuttingDown;  // Used as boolean - 0 = false, 1 = true

    SpinLock _cacheLock;
    typedef std::list<WiredTigerSession*> SessionCache;
    SessionCache _sessions;

    // Bumped when all open sessions need to be closed
    AtomicUInt64 _epoch;  // atomic so we can check it outside of the lock
};
}
