/** @file oplogreader.h */

/**
*    Copyright (C) 2012 10gen Inc.
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
*/


#pragma once

#include "../client/constants.h"
#include "dbhelpers.h"
#include "mongo/client/dbclientcursor.h"

namespace mongo {

    /* started abstracting out the querying of the primary/master's oplog
       still fairly awkward but a start.
    */

    class OplogReader {
        shared_ptr<DBClientConnection> _conn;
        shared_ptr<DBClientCursor> cursor;
        bool _doHandshake;
        int _tailingQueryOptions;
    public:
        OplogReader( bool doHandshake = true );
        ~OplogReader() { }
        void resetCursor() { cursor.reset(); }
        void resetConnection() {
            cursor.reset();
            _conn.reset();
        }
        DBClientConnection* conn() { return _conn.get(); }
        BSONObj findOne(const char *ns, const Query& q) {
            return conn()->findOne(ns, q, 0, QueryOption_SlaveOk);
        }
        BSONObj getLastOp(const char *ns) {
            return findOne(ns, Query().sort(reverseNaturalObj));
        }

        /* ok to call if already connected */
        bool connect(const std::string& hostname);

        bool connect(const BSONObj& rid, const int from, const string& to);

        void tailCheck() {
            if( cursor.get() && cursor->isDead() ) {
                log() << "repl: old cursor isDead, will initiate a new one" << endl;
                resetCursor();
            }
        }

        bool haveCursor() { return cursor.get() != 0; }

        /** this is ok but commented out as when used one should consider if QueryOption_OplogReplay
           is needed; if not fine, but if so, need to change.
        *//*
        void query(const char *ns, const BSONObj& query) {
            verify( !haveCursor() );
            cursor.reset( _conn->query(ns, query, 0, 0, 0, QueryOption_SlaveOk).release() );
        }*/

        /** this can be used; it is commented out as it does not indicate
            QueryOption_OplogReplay and that is likely important.  could be uncommented
            just need to add that.
            */
        /*
        void queryGTE(const char *ns, OpTime t) {
            BSONObjBuilder q;
            q.appendDate("$gte", t.asDate());
            BSONObjBuilder q2;
            q2.append("ts", q.done());
            query(ns, q2.done());
        }
        */

        void tailingQuery(const char *ns, const BSONObj& query, const BSONObj* fields=0);

        void tailingQueryGTE(const char *ns, OpTime t, const BSONObj* fields=0);

        /* Do a tailing query, but only send the ts field back. */
        void ghostQueryGTE(const char *ns, OpTime t) {
            const BSONObj fields = BSON("ts" << 1 << "_id" << 0);
            return tailingQueryGTE(ns, t, &fields);
        }

        bool more() {
            uassert( 15910, "Doesn't have cursor for reading oplog", cursor.get() );
            return cursor->more();
        }

        bool moreInCurrentBatch() {
            uassert( 15911, "Doesn't have cursor for reading oplog", cursor.get() );
            return cursor->moreInCurrentBatch();
        }

        int currentBatchMessageSize() {
            if( NULL == cursor->getMessage() )
                return 0;
            return cursor->getMessage()->size();
        }

        /* old mongod's can't do the await flag... */
        bool awaitCapable() {
            return cursor->hasResultFlag(ResultFlag_AwaitCapable);
        }

        int getTailingQueryOptions() const { return _tailingQueryOptions; }
        void setTailingQueryOptions( int tailingQueryOptions ) { _tailingQueryOptions = tailingQueryOptions; }

        void peek(vector<BSONObj>& v, int n) {
            if( cursor.get() )
                cursor->peek(v,n);
        }
        BSONObj nextSafe() { return cursor->nextSafe(); }
        BSONObj next() { return cursor->next(); }
        void putBack(BSONObj op) { cursor->putBack(op); }
        
    private:
        /** @return true iff connection was successful */ 
        bool commonConnect(const string& hostName);
        bool passthroughHandshake(const BSONObj& rid, const int f);
    };

}
