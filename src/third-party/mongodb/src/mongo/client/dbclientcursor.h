// file dbclientcursor.h

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

#include <boost/noncopyable.hpp>
#include <stack>

#include "mongo/client/dbclientinterface.h"
#include "mongo/client/export_macros.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/json.h"
#include "mongo/util/net/message.h"

namespace mongo {

class AScopedConnection;

/** for mock purposes only -- do not create variants of DBClientCursor, nor hang code here
    @see DBClientMockCursor
 */
class MONGO_CLIENT_API DBClientCursorInterface : boost::noncopyable {
public:
    virtual ~DBClientCursorInterface() {}
    virtual bool more() = 0;
    virtual BSONObj next() = 0;
    // TODO bring more of the DBClientCursor interface to here
protected:
    DBClientCursorInterface() {}
};

/** Queries return a cursor object */
class MONGO_CLIENT_API DBClientCursor : public DBClientCursorInterface {
public:
    /** If true, safe to call next().  Requests more from server if necessary. */
    bool more();

    /** If true, there is more in our local buffers to be fetched via next(). Returns
        false when a getMore request back to server would be required.  You can use this
        if you want to exhaust whatever data has been fetched to the client already but
        then perhaps stop.
    */
    int objsLeftInBatch() const {
        _assertIfNull();
        return _putBack.size() + batch.nReturned - batch.pos;
    }
    bool moreInCurrentBatch() {
        return objsLeftInBatch() > 0;
    }

    /** next
       @return next object in the result cursor.
       on an error at the remote server, you will get back:
         { $err: <std::string> }
       if you do not want to handle that yourself, call nextSafe().

       Warning: The returned BSONObj will become invalid after the next batch
           is fetched or when this cursor is destroyed.
    */
    BSONObj next();

    /**
        restore an object previously returned by next() to the cursor
     */
    void putBack(const BSONObj& o) {
        _putBack.push(o.getOwned());
    }

    /** throws AssertionException if get back { $err : ... } */
    BSONObj nextSafe();

    /** peek ahead at items buffered for future next() calls.
        never requests new data from the server.  so peek only effective
        with what is already buffered.
        WARNING: no support for _putBack yet!
    */
    void peek(std::vector<BSONObj>&, int atMost);

    // Peeks at first element, if exists
    BSONObj peekFirst();

    /**
     * peek ahead and see if an error occurred, and get the error if so.
     */
    bool peekError(BSONObj* error = NULL);

    /**
       iterate the rest of the cursor and return the number if items
     */
    int itcount() {
        int c = 0;
        while (more()) {
            next();
            c++;
        }
        return c;
    }

    /** cursor no longer valid -- use with tailable cursors.
       note you should only rely on this once more() returns false;
       'dead' may be preset yet some data still queued and locally
       available from the dbclientcursor.
    */
    bool isDead() const {
        return cursorId == 0;
    }

    bool tailable() const {
        return (opts & QueryOption_CursorTailable) != 0;
    }

    /** see ResultFlagType (constants.h) for flag values
        mostly these flags are for internal purposes -
        ResultFlag_ErrSet is the possible exception to that
    */
    bool hasResultFlag(int flag) {
        _assertIfNull();
        return (resultFlags & flag) != 0;
    }

    /// Change batchSize after construction. Can change after requesting first batch.
    void setBatchSize(int newBatchSize) {
        batchSize = newBatchSize;
    }

    DBClientCursor(DBClientBase* client,
                   const std::string& _ns,
                   BSONObj _query,
                   int _nToReturn,
                   int _nToSkip,
                   const BSONObj* _fieldsToReturn,
                   int queryOptions,
                   int bs)
        : _client(client),
          ns(_ns),
          query(_query),
          nToReturn(_nToReturn),
          haveLimit(_nToReturn > 0 && !(queryOptions & QueryOption_CursorTailable)),
          nToSkip(_nToSkip),
          fieldsToReturn(_fieldsToReturn),
          opts(queryOptions),
          batchSize(bs == 1 ? 2 : bs),
          resultFlags(0),
          cursorId(),
          _ownCursor(true),
          wasError(false) {
        _finishConsInit();
    }

    DBClientCursor(DBClientBase* client,
                   const std::string& _ns,
                   long long _cursorId,
                   int _nToReturn,
                   int options)
        : _client(client),
          ns(_ns),
          nToReturn(_nToReturn),
          haveLimit(_nToReturn > 0 && !(options & QueryOption_CursorTailable)),
          nToSkip(0),
          fieldsToReturn(0),
          opts(options),
          batchSize(0),
          resultFlags(0),
          cursorId(_cursorId),
          _ownCursor(true),
          wasError(false) {
        _finishConsInit();
    }

    virtual ~DBClientCursor();

    long long getCursorId() const {
        return cursorId;
    }

    /** by default we "own" the cursor and will send the server a KillCursor
        message when ~DBClientCursor() is called. This function overrides that.
    */
    void decouple() {
        _ownCursor = false;
    }

    void attach(AScopedConnection* conn);

    std::string originalHost() const {
        return _originalHost;
    }

    std::string getns() const {
        return ns;
    }

    Message* getMessage() {
        return batch.m.get();
    }

    /**
     * Used mainly to run commands on connections that doesn't support lazy initialization and
     * does not support commands through the call interface.
     *
     * @param cmd The BSON representation of the command to send.
     *
     * @return true if command was sent successfully
     */
    bool initCommand();

    /**
     * actually does the query
     */
    bool init();

    void initLazy(bool isRetry = false);
    bool initLazyFinish(bool& retry);

    class Batch : boost::noncopyable {
        friend class DBClientCursor;
        std::auto_ptr<Message> m;
        int nReturned;
        int pos;
        const char* data;

    public:
        Batch() : m(new Message()), nReturned(), pos(), data() {}
    };

    /**
     * Marks this object as dead and sends the KillCursors message to the server.
     *
     * Any errors that result from this are swallowed since this is typically performed as part of
     * cleanup and a failure to kill the cursor should not result in a failure of the operation
     * using the cursor.
     *
     * Killing an already killed or exhausted cursor does nothing, so it is safe to always call this
     * if you want to ensure that a cursor is killed.
     */
    void kill();

private:
    friend class DBClientBase;
    friend class DBClientConnection;

    int nextBatchSize();
    void _finishConsInit();

    Batch batch;
    DBClientBase* _client;
    std::string _originalHost;
    std::string ns;
    BSONObj query;
    int nToReturn;
    bool haveLimit;
    int nToSkip;
    const BSONObj* fieldsToReturn;
    int opts;
    int batchSize;
    std::stack<BSONObj> _putBack;
    int resultFlags;
    long long cursorId;
    bool _ownCursor;  // see decouple()
    std::string _scopedHost;
    std::string _lazyHost;
    bool wasError;

    void dataReceived() {
        bool retry;
        std::string lazyHost;
        dataReceived(retry, lazyHost);
    }
    void dataReceived(bool& retry, std::string& lazyHost);
    void requestMore();
    void exhaustReceiveMore();  // for exhaust

    // Don't call from a virtual function
    void _assertIfNull() const {
        uassert(13348, "connection died", this);
    }

    // non-copyable , non-assignable
    DBClientCursor(const DBClientCursor&);
    DBClientCursor& operator=(const DBClientCursor&);

    // init pieces
    void _assembleInit(Message& toSend);
};

/** iterate over objects in current batch only - will not cause a network call
 */
class MONGO_CLIENT_API DBClientCursorBatchIterator {
public:
    DBClientCursorBatchIterator(DBClientCursor& c) : _c(c), _n() {}
    bool moreInCurrentBatch() {
        return _c.moreInCurrentBatch();
    }
    BSONObj nextSafe() {
        massert(13383, "BatchIterator empty", moreInCurrentBatch());
        ++_n;
        return _c.nextSafe();
    }
    int n() const {
        return _n;
    }

private:
    DBClientCursor& _c;
    int _n;
};

}  // namespace mongo
