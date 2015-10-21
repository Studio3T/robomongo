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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kStorage

#include "mongo/platform/basic.h"

#include "rocks_recovery_unit.h"

#include <rocksdb/comparator.h>
#include <rocksdb/db.h>
#include <rocksdb/iterator.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>
#include <rocksdb/perf_context.h>
#include <rocksdb/write_batch.h>
#include <rocksdb/utilities/write_batch_with_index.h>

#include "mongo/base/checked_cast.h"
#include "mongo/db/concurrency/write_conflict_exception.h"
#include "mongo/db/operation_context.h"
#include "mongo/util/log.h"

#include "rocks_transaction.h"
#include "rocks_util.h"

namespace mongo {
namespace {
class PrefixStrippingIterator : public rocksdb::Iterator {
public:
    // baseIterator is consumed
    PrefixStrippingIterator(std::string prefix,
                            Iterator* baseIterator,
                            RocksCompactionScheduler* compactionScheduler,
                            std::unique_ptr<rocksdb::Slice> upperBound)
        : _rocksdbSkippedDeletionsInitial(0),
          _prefix(std::move(prefix)),
          _nextPrefix(std::move(rocksGetNextPrefix(_prefix))),
          _prefixSlice(_prefix.data(), _prefix.size()),
          _prefixSliceEpsilon(_prefix.data(), _prefix.size() + 1),
          _baseIterator(baseIterator),
          _compactionScheduler(compactionScheduler),
          _upperBound(std::move(upperBound)) {
        *_upperBound.get() = rocksdb::Slice(_nextPrefix);
    }

    virtual bool Valid() const {
        return _baseIterator->Valid() && _baseIterator->key().starts_with(_prefixSlice) &&
            _baseIterator->key().size() > _prefixSlice.size();
    }

    virtual void SeekToFirst() {
        startOp();
        // seek to first key bigger than prefix
        _baseIterator->Seek(_prefixSliceEpsilon);
        endOp();
    }
    virtual void SeekToLast() {
        startOp();
        // we can't have upper bound set to _nextPrefix since we need to seek to it
        *_upperBound.get() = rocksdb::Slice("\xFF\xFF\xFF\xFF");
        _baseIterator->Seek(_nextPrefix);
        // reset back to original value
        *_upperBound.get() = rocksdb::Slice(_nextPrefix);
        if (!_baseIterator->Valid()) {
            _baseIterator->SeekToLast();
        }
        if (_baseIterator->Valid() && !_baseIterator->key().starts_with(_prefixSlice)) {
            _baseIterator->Prev();
        }
        endOp();
    }

    virtual void Seek(const rocksdb::Slice& target) {
        startOp();
        std::unique_ptr<char[]> buffer(new char[_prefix.size() + target.size()]);
        memcpy(buffer.get(), _prefix.data(), _prefix.size());
        memcpy(buffer.get() + _prefix.size(), target.data(), target.size());
        _baseIterator->Seek(rocksdb::Slice(buffer.get(), _prefix.size() + target.size()));
        endOp();
    }

    virtual void Next() {
        startOp();
        _baseIterator->Next();
        endOp();
    }
    virtual void Prev() {
        startOp();
        _baseIterator->Prev();
        endOp();
    }

    virtual rocksdb::Slice key() const {
        rocksdb::Slice strippedKey = _baseIterator->key();
        strippedKey.remove_prefix(_prefix.size());
        return strippedKey;
    }
    virtual rocksdb::Slice value() const {
        return _baseIterator->value();
    }
    virtual rocksdb::Status status() const {
        return _baseIterator->status();
    }

private:
    void startOp() {
        if (_compactionScheduler == nullptr) {
            return;
        }
        if (rocksdb::GetPerfLevel() == rocksdb::PerfLevel::kDisable) {
            rocksdb::SetPerfLevel(rocksdb::kEnableCount);
        }
        _rocksdbSkippedDeletionsInitial = rocksdb::perf_context.internal_delete_skipped_count;
    }
    void endOp() {
        if (_compactionScheduler == nullptr) {
            return;
        }
        int skippedDeletionsOp =
            rocksdb::perf_context.internal_delete_skipped_count - _rocksdbSkippedDeletionsInitial;
        if (skippedDeletionsOp >= RocksCompactionScheduler::getSkippedDeletionsThreshold()) {
            _compactionScheduler->reportSkippedDeletionsAboveThreshold(_prefix);
        }
    }

    int _rocksdbSkippedDeletionsInitial;
    std::string _prefix;
    std::string _nextPrefix;
    rocksdb::Slice _prefixSlice;
    // the first possible key bigger than prefix. we use this for SeekToFirst()
    rocksdb::Slice _prefixSliceEpsilon;
    std::unique_ptr<Iterator> _baseIterator;
    // can be nullptr
    RocksCompactionScheduler* _compactionScheduler;  // not owned
    std::unique_ptr<rocksdb::Slice> _upperBound;
};

}  // anonymous namespace

std::atomic<int> RocksRecoveryUnit::_totalLiveRecoveryUnits(0);

RocksRecoveryUnit::RocksRecoveryUnit(RocksTransactionEngine* transactionEngine,
                                     rocksdb::DB* db,
                                     RocksCounterManager* counterManager,
                                     RocksCompactionScheduler* compactionScheduler,
                                     bool durable)
    : _transactionEngine(transactionEngine),
      _db(db),
      _counterManager(counterManager),
      _compactionScheduler(compactionScheduler),
      _durable(durable),
      _transaction(transactionEngine),
      _writeBatch(),
      _snapshot(NULL),
      _depth(0),
      _myTransactionCount(1) {
    RocksRecoveryUnit::_totalLiveRecoveryUnits.fetch_add(1, std::memory_order_relaxed);
}

RocksRecoveryUnit::~RocksRecoveryUnit() {
    _abort();
    RocksRecoveryUnit::_totalLiveRecoveryUnits.fetch_sub(1, std::memory_order_relaxed);
}

void RocksRecoveryUnit::beginUnitOfWork(OperationContext* opCtx) {
    _depth++;
}

void RocksRecoveryUnit::commitUnitOfWork() {
    if (_depth > 1) {
        return;  // only outermost gets committed.
    }

    if (_writeBatch) {
        _commit();
    }

    try {
        for (Changes::const_iterator it = _changes.begin(), end = _changes.end(); it != end; ++it) {
            (*it)->commit();
        }
        _changes.clear();
    } catch (...) {
        std::terminate();
    }

    _releaseSnapshot();
}

void RocksRecoveryUnit::endUnitOfWork() {
    _depth--;
    if (_depth == 0) {
        _abort();
    }
}

bool RocksRecoveryUnit::awaitCommit() {
    // Not sure what we should do here. awaitCommit() is called when WriteConcern is FSYNC or
    // JOURNAL. In our case, we're doing JOURNAL WriteConcern for each transaction (no matter
    // WriteConcern). However, if WriteConcern is FSYNC we should probably call Write() with
    // sync option. So far we're just not doing anything. In the future we should figure which
    // of the WriteConcerns is this (FSYNC or JOURNAL) and then if it's FSYNC do something
    // special.
    return true;
}

void RocksRecoveryUnit::commitAndRestart() {
    invariant(_depth == 0);
    commitUnitOfWork();
}

// lazily initialized because Recovery Units are sometimes initialized just for reading,
// which does not require write batches
rocksdb::WriteBatchWithIndex* RocksRecoveryUnit::writeBatch() {
    if (!_writeBatch) {
        // this assumes that default column family uses default comparator. change this if you
        // change default column family's comparator
        _writeBatch.reset(new rocksdb::WriteBatchWithIndex(rocksdb::BytewiseComparator(), 0, true));
    }

    return _writeBatch.get();
}

void RocksRecoveryUnit::setOplogReadTill(const RecordId& record) {
    _oplogReadTill = record;
}

void RocksRecoveryUnit::registerChange(Change* change) {
    _changes.push_back(change);
}

SnapshotId RocksRecoveryUnit::getSnapshotId() const {
    return SnapshotId(_myTransactionCount);
}

void RocksRecoveryUnit::_releaseSnapshot() {
    if (_snapshot) {
        _transaction.abort();
        _db->ReleaseSnapshot(_snapshot);
        _snapshot = nullptr;
    }
    _myTransactionCount++;
}

void RocksRecoveryUnit::_commit() {
    invariant(_writeBatch);
    rocksdb::WriteBatch* wb = _writeBatch->GetWriteBatch();
    for (auto pair : _deltaCounters) {
        auto& counter = pair.second;
        counter._value->fetch_add(counter._delta, std::memory_order::memory_order_relaxed);
        long long newValue = counter._value->load(std::memory_order::memory_order_relaxed);
        _counterManager->updateCounter(pair.first, newValue, wb);
    }

    if (wb->Count() != 0) {
        // Order of operations here is important. It needs to be synchronized with
        // _transaction.recordSnapshotId() and _db->GetSnapshot() and
        rocksdb::WriteOptions writeOptions;
        writeOptions.disableWAL = !_durable;
        auto status = _db->Write(rocksdb::WriteOptions(), wb);
        invariantRocksOK(status);
        _transaction.commit();
    }
    _deltaCounters.clear();
    _writeBatch.reset();
}

void RocksRecoveryUnit::_abort() {
    try {
        for (Changes::const_reverse_iterator it = _changes.rbegin(), end = _changes.rend();
             it != end;
             ++it) {
            (*it)->rollback();
        }
        _changes.clear();
    } catch (...) {
        std::terminate();
    }

    _deltaCounters.clear();
    _writeBatch.reset();

    _releaseSnapshot();
}

const rocksdb::Snapshot* RocksRecoveryUnit::snapshot() {
    if (!_snapshot) {
        // Order of operations here is important. It needs to be synchronized with
        // _db->Write() and _transaction.commit()
        _transaction.recordSnapshotId();
        _snapshot = _db->GetSnapshot();
    }

    return _snapshot;
}

rocksdb::Status RocksRecoveryUnit::Get(const rocksdb::Slice& key, std::string* value) {
    if (_writeBatch && _writeBatch->GetWriteBatch()->Count() > 0) {
        boost::scoped_ptr<rocksdb::WBWIIterator> wb_iterator(_writeBatch->NewIterator());
        wb_iterator->Seek(key);
        if (wb_iterator->Valid() && wb_iterator->Entry().key == key) {
            const auto& entry = wb_iterator->Entry();
            if (entry.type == rocksdb::WriteType::kDeleteRecord) {
                return rocksdb::Status::NotFound();
            }
            *value = std::string(entry.value.data(), entry.value.size());
            return rocksdb::Status::OK();
        }
    }
    rocksdb::ReadOptions options;
    options.snapshot = snapshot();
    return _db->Get(options, key, value);
}

rocksdb::Iterator* RocksRecoveryUnit::NewIterator(std::string prefix, bool isOplog) {
    std::unique_ptr<rocksdb::Slice> upperBound(new rocksdb::Slice());
    rocksdb::ReadOptions options;
    options.iterate_upper_bound = upperBound.get();
    options.snapshot = snapshot();
    auto iterator = _db->NewIterator(options);
    if (_writeBatch && _writeBatch->GetWriteBatch()->Count() > 0) {
        iterator = _writeBatch->NewIteratorWithBase(iterator);
    }
    return new PrefixStrippingIterator(std::move(prefix),
                                       iterator,
                                       isOplog ? nullptr : _compactionScheduler,
                                       std::move(upperBound));
}

rocksdb::Iterator* RocksRecoveryUnit::NewIteratorNoSnapshot(rocksdb::DB* db, std::string prefix) {
    std::unique_ptr<rocksdb::Slice> upperBound(new rocksdb::Slice());
    rocksdb::ReadOptions options;
    options.iterate_upper_bound = upperBound.get();
    auto iterator = db->NewIterator(options);
    return new PrefixStrippingIterator(std::move(prefix), iterator, nullptr, std::move(upperBound));
}

void RocksRecoveryUnit::incrementCounter(const rocksdb::Slice& counterKey,
                                         std::atomic<long long>* counter,
                                         long long delta) {
    if (delta == 0) {
        return;
    }

    auto pair = _deltaCounters.find(counterKey.ToString());
    if (pair == _deltaCounters.end()) {
        _deltaCounters[counterKey.ToString()] = mongo::RocksRecoveryUnit::Counter(counter, delta);
    } else {
        pair->second._delta += delta;
    }
}

long long RocksRecoveryUnit::getDeltaCounter(const rocksdb::Slice& counterKey) {
    auto counter = _deltaCounters.find(counterKey.ToString());
    if (counter == _deltaCounters.end()) {
        return 0;
    } else {
        return counter->second._delta;
    }
}

RocksRecoveryUnit* RocksRecoveryUnit::getRocksRecoveryUnit(OperationContext* opCtx) {
    return checked_cast<RocksRecoveryUnit*>(opCtx->recoveryUnit());
}
}
