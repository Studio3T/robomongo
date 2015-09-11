// in_memory_record_store.h

/**
*    Copyright (C) 2014 MongoDB Inc.
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

#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <map>

#include "mongo/db/storage/capped_callback.h"
#include "mongo/db/storage/record_store.h"

namespace mongo {

class InMemoryRecordIterator;

/**
 * A RecordStore that stores all data in-memory.
 *
 * @param cappedMaxSize - required if isCapped. limit uses dataSize() in this impl.
 */
class InMemoryRecordStore : public RecordStore {
public:
    explicit InMemoryRecordStore(const StringData& ns,
                                 boost::shared_ptr<void>* dataInOut,
                                 bool isCapped = false,
                                 int64_t cappedMaxSize = -1,
                                 int64_t cappedMaxDocs = -1,
                                 CappedDocumentDeleteCallback* cappedDeleteCallback = NULL);

    virtual const char* name() const;

    virtual RecordData dataFor(OperationContext* txn, const RecordId& loc) const;

    virtual bool findRecord(OperationContext* txn, const RecordId& loc, RecordData* rd) const;

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

    virtual RecordIterator* getIterator(OperationContext* txn,
                                        const RecordId& start,
                                        const CollectionScanParams::Direction& dir) const;

    virtual RecordIterator* getIteratorForRepair(OperationContext* txn) const;

    virtual std::vector<RecordIterator*> getManyIterators(OperationContext* txn) const;

    virtual Status truncate(OperationContext* txn);

    virtual void temp_cappedTruncateAfter(OperationContext* txn, RecordId end, bool inclusive);

    virtual Status validate(OperationContext* txn,
                            bool full,
                            bool scanData,
                            ValidateAdaptor* adaptor,
                            ValidateResults* results,
                            BSONObjBuilder* output);

    virtual void appendCustomStats(OperationContext* txn,
                                   BSONObjBuilder* result,
                                   double scale) const;

    virtual Status touch(OperationContext* txn, BSONObjBuilder* output) const;

    virtual void increaseStorageSize(OperationContext* txn, int size, bool enforceQuota);

    virtual int64_t storageSize(OperationContext* txn,
                                BSONObjBuilder* extraInfo = NULL,
                                int infoLevel = 0) const;

    virtual long long dataSize(OperationContext* txn) const {
        return _data->dataSize;
    }

    virtual long long numRecords(OperationContext* txn) const {
        return _data->records.size();
    }

    virtual boost::optional<RecordId> oplogStartHack(OperationContext* txn,
                                                     const RecordId& startingPosition) const;

    virtual void updateStatsAfterRepair(OperationContext* txn,
                                        long long numRecords,
                                        long long dataSize) {
        invariant(_data->records.size() == size_t(numRecords));
        _data->dataSize = dataSize;
    }

protected:
    struct InMemoryRecord {
        InMemoryRecord() : size(0) {}
        InMemoryRecord(int size) : size(size), data(new char[size]) {}

        RecordData toRecordData() const {
            return RecordData(data.get(), size);
        }

        int size;
        boost::shared_array<char> data;
    };

    virtual const InMemoryRecord* recordFor(const RecordId& loc) const;
    virtual InMemoryRecord* recordFor(const RecordId& loc);

public:
    //
    // Not in RecordStore interface
    //

    typedef std::map<RecordId, InMemoryRecord> Records;

    bool isCapped() const {
        return _isCapped;
    }
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

private:
    class InsertChange;
    class RemoveChange;
    class TruncateChange;

    StatusWith<RecordId> extractAndCheckLocForOplog(const char* data, int len) const;

    RecordId allocateLoc();
    bool cappedAndNeedDelete(OperationContext* txn) const;
    void cappedDeleteAsNeeded(OperationContext* txn);

    // TODO figure out a proper solution to metadata
    const bool _isCapped;
    const int64_t _cappedMaxSize;
    const int64_t _cappedMaxDocs;
    CappedDocumentDeleteCallback* _cappedDeleteCallback;

    // This is the "persistent" data.
    struct Data {
        Data(bool isOplog) : dataSize(0), nextId(1), isOplog(isOplog) {}

        int64_t dataSize;
        Records records;
        int64_t nextId;
        const bool isOplog;
    };

    Data* const _data;
};

class InMemoryRecordIterator : public RecordIterator {
public:
    InMemoryRecordIterator(OperationContext* txn,
                           const InMemoryRecordStore::Records& records,
                           const InMemoryRecordStore& rs,
                           RecordId start = RecordId(),
                           bool tailable = false);

    virtual bool isEOF();

    virtual RecordId curr();

    virtual RecordId getNext();

    virtual void invalidate(const RecordId& dl);

    virtual void saveState();

    virtual bool restoreState(OperationContext* txn);

    virtual RecordData dataFor(const RecordId& loc) const;

private:
    OperationContext* _txn;  // not owned
    InMemoryRecordStore::Records::const_iterator _it;
    bool _tailable;
    RecordId _lastLoc;  // only for restarting tailable
    bool _killedByInvalidate;

    const InMemoryRecordStore::Records& _records;
    const InMemoryRecordStore& _rs;
};

class InMemoryRecordReverseIterator : public RecordIterator {
public:
    InMemoryRecordReverseIterator(OperationContext* txn,
                                  const InMemoryRecordStore::Records& records,
                                  const InMemoryRecordStore& rs,
                                  RecordId start = RecordId());

    virtual bool isEOF();

    virtual RecordId curr();

    virtual RecordId getNext();

    virtual void invalidate(const RecordId& dl);

    virtual void saveState();

    virtual bool restoreState(OperationContext* txn);

    virtual RecordData dataFor(const RecordId& loc) const;

private:
    OperationContext* _txn;  // not owned
    InMemoryRecordStore::Records::const_reverse_iterator _it;
    bool _killedByInvalidate;
    RecordId _savedLoc;  // isNull if saved at EOF

    const InMemoryRecordStore::Records& _records;
    const InMemoryRecordStore& _rs;
};

}  // namespace mongo
