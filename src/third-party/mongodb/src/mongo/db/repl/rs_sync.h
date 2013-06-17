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
 */

#pragma once

#include <deque>
#include <vector>

#include "mongo/db/client.h"
#include "mongo/db/dur.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/oplog.h"
#include "mongo/util/concurrency/thread_pool.h"

namespace mongo {
namespace replset {

    class BackgroundSyncInterface;

    /**
     * "Normal" replica set syncing
     */
    class SyncTail : public Sync {
        typedef void (*MultiSyncApplyFunc)(const std::vector<BSONObj>& ops, SyncTail* st);
    public:
        SyncTail(BackgroundSyncInterface *q);
        virtual ~SyncTail();
        virtual bool syncApply(const BSONObj &o, bool convertUpdateToUpsert = false);

        /**
         * Apply ops from applyGTEObj's ts to at least minValidObj's ts.  Note that, due to
         * batching, this may end up applying ops beyond minValidObj's ts.
         *
         * @param applyGTEObj the op to start replicating at.  This is actually not used except in
         *                    comparison to minValidObj: the background sync thread keeps its own
         *                    record of where we're synced to and starts providing ops from that
         *                    point.
         * @param minValidObj the op to finish syncing at.  This function cannot return (other than
         *                    fatally erroring out) without applying at least this op.
         * @param func        whether this should use initial sync logic (recloning docs) or
         *                    "normal" logic.
         * @return BSONObj    the op that was synced to.  This may be greater than minValidObj, as a
         *                    single batch might blow right by minvalid. If applyGTEObj is the same
         *                    op as minValidObj, this will be applyGTEObj.
         */
        BSONObj oplogApplySegment(const BSONObj& applyGTEObj, const BSONObj& minValidObj,
                               MultiSyncApplyFunc func);

        /**
         * Runs oplogApplySegment without allowing recloning documents.
         */
        virtual BSONObj oplogApplication(const BSONObj& applyGTEObj, const BSONObj& minValidObj);

        void oplogApplication();
        bool peek(BSONObj* obj);

        class OpQueue {
        public:
            OpQueue() : _size(0) {}
            size_t getSize() { return _size; }
            std::deque<BSONObj>& getDeque() { return _deque; }
            void push_back(BSONObj& op) {
                _deque.push_back(op);
                _size += op.objsize();
            }
            bool empty() {
                return _deque.empty();
            }
        private:
            std::deque<BSONObj> _deque;
            size_t _size;
        };

        // returns true if we should continue waiting for BSONObjs, false if we should
        // stop waiting and apply the queue we have.  Only returns false if !ops.empty().
        bool tryPopAndWaitForMore(OpQueue* ops);
        
        // After ops have been written to db, call this
        // to update local oplog.rs, as well as notify the primary
        // that we have applied the ops.
        // Ops are removed from the deque.
        void applyOpsToOplog(std::deque<BSONObj>* ops);

    protected:
        // Cap the batches using the limit on journal commits.
        // This works out to be 100 MB (64 bit) or 50 MB (32 bit)
        static const unsigned int replBatchLimitBytes = dur::UncommittedBytesLimit;
        static const int replBatchLimitSeconds = 1;
        static const unsigned int replBatchLimitOperations = 5000;

        // Prefetch and write a deque of operations, using the supplied function.
        // Initial Sync and Sync Tail each use a different function.
        void multiApply(std::deque<BSONObj>& ops, MultiSyncApplyFunc applyFunc);

        // The version of the last op to be read
        int oplogVersion;

    private:
        BackgroundSyncInterface* _networkQueue;

        // Doles out all the work to the reader pool threads and waits for them to complete
        void prefetchOps(const std::deque<BSONObj>& ops);
        // Used by the thread pool readers to prefetch an op
        static void prefetchOp(const BSONObj& op);

        // Doles out all the work to the writer pool threads and waits for them to complete
        void applyOps(const std::vector< std::vector<BSONObj> >& writerVectors, 
                      MultiSyncApplyFunc applyFunc);

        void fillWriterVectors(const std::deque<BSONObj>& ops, 
                               std::vector< std::vector<BSONObj> >* writerVectors);
        void handleSlaveDelay(const BSONObj& op);
        void setOplogVersion(const BSONObj& op);
    };

    /**
     * Initial clone and sync
     */
    class InitialSync : public SyncTail {
    public:
        virtual ~InitialSync();
        InitialSync(BackgroundSyncInterface *q);

        /**
         * Creates the initial oplog entry: applies applyGTEObj and writes it to the oplog.  Then
         * this runs oplogApplySegment allowing recloning documents.
         */
        BSONObj oplogApplication(const BSONObj& applyGTEObj, const BSONObj& minValidObj);
    };

    // TODO: move hbmsg into an error-keeping class (SERVER-4444)
    void sethbmsg(const string& s, const int logLevel=0);

    // These free functions are used by the thread pool workers to write ops to the db.
    void multiSyncApply(const std::vector<BSONObj>& ops, SyncTail* st);
    void multiInitialSyncApply(const std::vector<BSONObj>& ops, SyncTail* st);

} // namespace replset
} // namespace mongo
