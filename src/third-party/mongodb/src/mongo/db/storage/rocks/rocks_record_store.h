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

#include <atomic>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <memory>
#include <vector>
#include <functional>

#include <rocksdb/options.h>

#include "mongo/db/storage/capped_callback.h"
#include "mongo/db/storage/record_store.h"
#include "mongo/platform/atomic_word.h"
#include "mongo/util/timer.h"

namespace rocksdb {
class DB;
class Iterator;
class Slice;
}

namespace mongo {

class RocksCounterManager;
class CappedVisibilityManager {
public:
    CappedVisibilityManager() : _oplog_highestSeen(RecordId::min()) {}
    void dealtWithCappedRecord(const RecordId& record);
    void updateHighestSeen(const RecordId& record);
    void addUncommittedRecord(OperationContext* txn, const RecordId& record);

    // a bit hacky function, but does the job
    RecordId getNextAndAddUncommittedRecord(OperationContext* txn,
                                            std::function<RecordId()> nextId);

    bool isCappedHidden(const RecordId& record) const;
    RecordId oplogStartHack() const;

private:
    void _addUncommittedRecord_inlock(OperationContext* txn, const RecordId& record);

    // protects the state
    mutable boost::mutex _lock;
    std::vector<RecordId> _uncommittedRecords;
    RecordId _oplog_highestSeen;
};

class RocksRecoveryUnit;
class RocksOplogKeyTracker;

class RocksRecordStore : public RecordStore {
public:
    RocksRecordStore(const StringData& ns,
                     const StringData& id,
                     rocksdb::DB* db,
                     RocksCounterManager* counterManager,
                     std::string prefix,
                     bool isCapped = false,
                     int64_t cappedMaxSize = -1,
                     int64_t cappedMaxDocs = -1,
                     CappedDocumentDeleteCallback* cappedDeleteCallback = NULL);

    virtual ~RocksRecordStore();

    // name of the RecordStore implementation
    virtual const char* name() const {
        return "rocks";
    }

    virtual long long dataSize(OperationContext* txn) const;

    virtual long long numRecords(OperationContext* txn) const;

    virtual bool isCapped() const {
        return _isCapped;
    }

    virtual int64_t storageSize(OperationContext* txn,
                                BSONObjBuilder* extraInfo = NULL,
                                int infoLevel = 0) const;

    // CRUD related

    virtual RecordData dataFor(OperationContext* txn, const RecordId& loc) const;

    virtual bool findRecord(OperationContext* txn, const RecordId& loc, RecordData* out) const;

    virtual void deleteRecord(OperationContext* txn, const RecordId& dl);

    virtual StatusWith<RecordId> insertRecord(OperationContext* txn,
                                              const char* data,
                                              int len,
                                              bool enforceQuota);

    virtual StatusWith<RecordId> insertRecord(OperationContext* txn,
                                              const DocWriter* doc,
                                              bool enforceQuota);

    virtual StatusWith<RecordId> updateRecord(OperationContext* txn,
                                              const RecordId& oldLocation,
                                              const char* data,
                                              int len,
                                              bool enforceQuota,
                                              UpdateNotifier* notifier);

    virtual bool updateWithDamagesSupported() const;

    virtual Status updateWithDamages(OperationContext* txn,
                                     const RecordId& loc,
                                     const RecordData& oldRec,
                                     const char* damageSource,
                                     const mutablebson::DamageVector& damages);

    virtual RecordIterator* getIterator(
        OperationContext* txn,
        const RecordId& start = RecordId(),
        const CollectionScanParams::Direction& dir = CollectionScanParams::FORWARD) const;

    virtual std::vector<RecordIterator*> getManyIterators(OperationContext* txn) const;

    virtual Status truncate(OperationContext* txn);

    virtual bool compactSupported() const {
        return true;
    }
    virtual bool compactsInPlace() const {
        return true;
    }

    virtual Status compact(OperationContext* txn,
                           RecordStoreCompactAdaptor* adaptor,
                           const CompactOptions* options,
                           CompactStats* stats);

    virtual Status validate(OperationContext* txn,
                            bool full,
                            bool scanData,
                            ValidateAdaptor* adaptor,
                            ValidateResults* results,
                            BSONObjBuilder* output);

    virtual void appendCustomStats(OperationContext* txn,
                                   BSONObjBuilder* result,
                                   double scale) const;

    virtual void temp_cappedTruncateAfter(OperationContext* txn, RecordId end, bool inclusive);

    virtual boost::optional<RecordId> oplogStartHack(OperationContext* txn,
                                                     const RecordId& startingPosition) const;

    virtual Status oplogDiskLocRegister(OperationContext* txn, const OpTime& opTime);

    virtual void updateStatsAfterRepair(OperationContext* txn,
                                        long long numRecords,
                                        long long dataSize);

    void setCappedDeleteCallback(CappedDocumentDeleteCallback* cb) {
        _cappedDeleteCallback = cb;
    }
    bool cappedMaxDocs() const {
        invariant(_isCapped);
        return _cappedMaxDocs;
    }
    bool cappedMaxSize() const {
        invariant(_isCapped);
        return _cappedMaxSize;
    }
    bool isOplog() const {
        return _isOplog;
    }

    int64_t cappedDeleteAsNeeded(OperationContext* txn, const RecordId& justInserted);
    int64_t cappedDeleteAsNeeded_inlock(OperationContext* txn, const RecordId& justInserted);
    boost::timed_mutex& cappedDeleterMutex() {
        return _cappedDeleterMutex;
    }

    static rocksdb::Comparator* newRocksCollectionComparator();

private:
    // we just need to expose _makePrefixedKey to RocksOplogKeyTracker
    friend class RocksOplogKeyTracker;
    // NOTE: RecordIterator might outlive the RecordStore. That's why we use all those
    // shared_ptrs
    class Iterator : public RecordIterator {
    public:
        Iterator(OperationContext* txn,
                 rocksdb::DB* db,
                 std::string prefix,
                 boost::shared_ptr<CappedVisibilityManager> cappedVisibilityManager,
                 const CollectionScanParams::Direction& dir,
                 const RecordId& start);

        virtual bool isEOF();
        virtual RecordId curr();
        virtual RecordId getNext();
        virtual void invalidate(const RecordId& dl);
        virtual void saveState();
        virtual bool restoreState(OperationContext* txn);
        virtual RecordData dataFor(const RecordId& loc) const;

    private:
        void _locate(const RecordId& loc);
        RecordId _decodeCurr() const;
        bool _forward() const;

        OperationContext* _txn;
        rocksdb::DB* _db;  // not owned
        std::string _prefix;
        boost::shared_ptr<CappedVisibilityManager> _cappedVisibilityManager;
        CollectionScanParams::Direction _dir;
        bool _eof;
        const RecordId _readUntilForOplog;
        RecordId _curr;
        RecordId _lastLoc;
        boost::scoped_ptr<rocksdb::Iterator> _iterator;
    };

    static RecordId _makeRecordId(const rocksdb::Slice& slice);

    static RecordData _getDataFor(rocksdb::DB* db,
                                  const std::string& prefix,
                                  OperationContext* txn,
                                  const RecordId& loc);

    RecordId _nextId();
    bool cappedAndNeedDelete(long long dataSizeDelta, long long numRecordsDelta) const;

    // The use of this function requires that the passed in storage outlives the returned Slice
    static rocksdb::Slice _makeKey(const RecordId& loc, int64_t* storage);
    static std::string _makePrefixedKey(const std::string& prefix, const RecordId& loc);

    void _changeNumRecords(OperationContext* txn, int64_t amount);
    void _increaseDataSize(OperationContext* txn, int64_t amount);

    rocksdb::DB* _db;                      // not owned
    RocksCounterManager* _counterManager;  // not owned
    std::string _prefix;

    const bool _isCapped;
    const int64_t _cappedMaxSize;
    const int64_t _cappedMaxSizeSlack;  // when to start applying backpressure
    const int64_t _cappedMaxDocs;
    CappedDocumentDeleteCallback* _cappedDeleteCallback;
    mutable boost::timed_mutex _cappedDeleterMutex;  // see comment in ::cappedDeleteAsNeeded
    int _cappedDeleteCheckCount;                     // see comment in ::cappedDeleteAsNeeded

    const bool _isOplog;
    // nullptr iff _isOplog == false
    RocksOplogKeyTracker* _oplogKeyTracker;
    // keep track of when we compacted oplog last time. only valid when _isOplog == true.
    // Protected by _cappedDeleterMutex.
    Timer _oplogSinceLastCompaction;
    // compact oplog every 30 min
    static const int kOplogCompactEveryMins = 30;

    // invariant: there is no live records earlier than _cappedOldestKeyHint. There might be
    // some records that are dead after _cappedOldestKeyHint.
    // SeekToFirst() on an capped collection is an expensive operation because bunch of keys at
    // the start are deleted. To reduce the overhead, we remember the next key to delete and
    // seek directly to it. This will not work correctly if somebody inserted a key before this
    // _cappedOldestKeyHint. However, we prevent this from happening by using
    // _cappedVisibilityManager and checking isCappedHidden() during deletions
    RecordId _cappedOldestKeyHint;

    boost::shared_ptr<CappedVisibilityManager> _cappedVisibilityManager;

    std::string _ident;
    AtomicUInt64 _nextIdNum;
    std::atomic<long long> _dataSize;
    std::atomic<long long> _numRecords;

    const std::string _dataSizeKey;
    const std::string _numRecordsKey;

    bool _shuttingDown;
    bool _hasBackgroundThread;

    /**
     * During record store creation, if a record count is under
     * 'kCollectionScanOnCreationThreshold', perform a collection scan to update the
     * number of records and data size counters
     */
    static const long long kCollectionScanOnCreationThreshold = 10000;
};
}
