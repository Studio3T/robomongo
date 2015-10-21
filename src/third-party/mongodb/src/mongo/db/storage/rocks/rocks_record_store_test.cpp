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

#include "mongo/platform/basic.h"

#include <boost/filesystem/operations.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <memory>
#include <vector>

#include <rocksdb/comparator.h>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>

#include "mongo/db/concurrency/write_conflict_exception.h"
#include "mongo/db/storage/record_store_test_harness.h"
#include "mongo/unittest/unittest.h"
#include "mongo/unittest/temp_dir.h"

#include "rocks_record_store.h"
#include "rocks_recovery_unit.h"
#include "rocks_transaction.h"

namespace mongo {

using boost::scoped_ptr;
using boost::shared_ptr;
using std::string;

class RocksRecordStoreHarnessHelper : public HarnessHelper {
public:
    RocksRecordStoreHarnessHelper() : _tempDir(_testNamespace) {
        boost::filesystem::remove_all(_tempDir.path());
        rocksdb::DB* db;
        rocksdb::Options options;
        options.create_if_missing = true;
        auto s = rocksdb::DB::Open(options, _tempDir.path(), &db);
        ASSERT(s.ok());
        _db.reset(db);
        _counterManager.reset(new RocksCounterManager(_db.get(), true));
    }

    virtual RecordStore* newNonCappedRecordStore() {
        return newNonCappedRecordStore("foo.bar");
    }
    RecordStore* newNonCappedRecordStore(const std::string& ns) {
        return new RocksRecordStore(ns, "1", _db.get(), _counterManager.get(), "prefix");
    }

    RecordStore* newCappedRecordStore(const std::string& ns,
                                      int64_t cappedMaxSize,
                                      int64_t cappedMaxDocs) {
        return new RocksRecordStore(ns,
                                    "1",
                                    _db.get(),
                                    _counterManager.get(),
                                    "prefix",
                                    true,
                                    cappedMaxSize,
                                    cappedMaxDocs);
    }

    virtual RecoveryUnit* newRecoveryUnit() {
        return new RocksRecoveryUnit(
            &_transactionEngine, _db.get(), _counterManager.get(), nullptr, true);
    }

private:
    string _testNamespace = "mongo-rocks-record-store-test";
    unittest::TempDir _tempDir;
    boost::scoped_ptr<rocksdb::DB> _db;
    RocksTransactionEngine _transactionEngine;
    scoped_ptr<RocksCounterManager> _counterManager;
};

HarnessHelper* newHarnessHelper() {
    return new RocksRecordStoreHarnessHelper();
}

TEST(RocksRecordStoreTest, Isolation1) {
    scoped_ptr<HarnessHelper> harnessHelper(newHarnessHelper());
    scoped_ptr<RecordStore> rs(harnessHelper->newNonCappedRecordStore());

    RecordId loc1;
    RecordId loc2;

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            WriteUnitOfWork uow(opCtx.get());

            StatusWith<RecordId> res = rs->insertRecord(opCtx.get(), "a", 2, false);
            ASSERT_OK(res.getStatus());
            loc1 = res.getValue();

            res = rs->insertRecord(opCtx.get(), "a", 2, false);
            ASSERT_OK(res.getStatus());
            loc2 = res.getValue();

            uow.commit();
        }
    }

    {
        scoped_ptr<OperationContext> t1(harnessHelper->newOperationContext());
        scoped_ptr<OperationContext> t2(harnessHelper->newOperationContext());

        scoped_ptr<WriteUnitOfWork> w1(new WriteUnitOfWork(t1.get()));
        scoped_ptr<WriteUnitOfWork> w2(new WriteUnitOfWork(t2.get()));

        rs->dataFor(t1.get(), loc1);
        rs->dataFor(t2.get(), loc1);

        ASSERT_OK(rs->updateRecord(t1.get(), loc1, "b", 2, false, NULL).getStatus());
        ASSERT_OK(rs->updateRecord(t1.get(), loc2, "B", 2, false, NULL).getStatus());

        // this should throw
        ASSERT_THROWS(rs->updateRecord(t2.get(), loc1, "c", 2, false, NULL),
                      WriteConflictException);

        w1->commit();  // this should succeed
    }
}

TEST(RocksRecordStoreTest, Isolation2) {
    scoped_ptr<HarnessHelper> harnessHelper(newHarnessHelper());
    scoped_ptr<RecordStore> rs(harnessHelper->newNonCappedRecordStore());

    RecordId loc1;
    RecordId loc2;

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            WriteUnitOfWork uow(opCtx.get());

            StatusWith<RecordId> res = rs->insertRecord(opCtx.get(), "a", 2, false);
            ASSERT_OK(res.getStatus());
            loc1 = res.getValue();

            res = rs->insertRecord(opCtx.get(), "a", 2, false);
            ASSERT_OK(res.getStatus());
            loc2 = res.getValue();

            uow.commit();
        }
    }

    {
        scoped_ptr<OperationContext> t1(harnessHelper->newOperationContext());
        scoped_ptr<OperationContext> t2(harnessHelper->newOperationContext());

        // ensure we start transactions
        rs->dataFor(t1.get(), loc2);
        rs->dataFor(t2.get(), loc2);

        {
            WriteUnitOfWork w(t1.get());
            ASSERT_OK(rs->updateRecord(t1.get(), loc1, "b", 2, false, NULL).getStatus());
            w.commit();
        }

        {
            WriteUnitOfWork w(t2.get());
            ASSERT_EQUALS(string("a"), rs->dataFor(t2.get(), loc1).data());
            // this should fail as our version of loc1 is too old
            ASSERT_THROWS(rs->updateRecord(t2.get(), loc1, "c", 2, false, NULL),
                          WriteConflictException);
        }
    }
}

StatusWith<RecordId> insertBSON(scoped_ptr<OperationContext>& opCtx,
                                scoped_ptr<RecordStore>& rs,
                                const OpTime& opTime) {
    BSONObj obj = BSON("ts" << opTime);
    WriteUnitOfWork wuow(opCtx.get());
    RocksRecordStore* rrs = dynamic_cast<RocksRecordStore*>(rs.get());
    invariant(rrs);
    Status status = rrs->oplogDiskLocRegister(opCtx.get(), opTime);
    if (!status.isOK())
        return StatusWith<RecordId>(status);
    StatusWith<RecordId> res = rs->insertRecord(opCtx.get(), obj.objdata(), obj.objsize(), false);
    if (res.isOK())
        wuow.commit();
    return res;
}

TEST(RocksRecordStoreTest, OplogHack) {
    RocksRecordStoreHarnessHelper harnessHelper;
    scoped_ptr<RecordStore> rs(harnessHelper.newNonCappedRecordStore("local.oplog.foo"));
    {
        scoped_ptr<OperationContext> opCtx(harnessHelper.newOperationContext());

        // always illegal
        ASSERT_EQ(insertBSON(opCtx, rs, OpTime(2, -1)).getStatus(), ErrorCodes::BadValue);

        {
            BSONObj obj = BSON("not_ts" << OpTime(2, 1));
            ASSERT_EQ(
                rs->insertRecord(opCtx.get(), obj.objdata(), obj.objsize(), false).getStatus(),
                ErrorCodes::BadValue);

            obj = BSON("ts"
                       << "not an OpTime");
            ASSERT_EQ(
                rs->insertRecord(opCtx.get(), obj.objdata(), obj.objsize(), false).getStatus(),
                ErrorCodes::BadValue);
        }

        // currently dasserts
        // ASSERT_EQ(insertBSON(opCtx, rs, BSON("ts" << OpTime(-2,1))).getStatus(),
        // ErrorCodes::BadValue);

        // success cases
        ASSERT_EQ(insertBSON(opCtx, rs, OpTime(1, 1)).getValue(), RecordId(1, 1));

        ASSERT_EQ(insertBSON(opCtx, rs, OpTime(1, 2)).getValue(), RecordId(1, 2));

        ASSERT_EQ(insertBSON(opCtx, rs, OpTime(2, 2)).getValue(), RecordId(2, 2));
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper.newOperationContext());
        // find start
        ASSERT_EQ(rs->oplogStartHack(opCtx.get(), RecordId(0, 1)), RecordId());      // nothing <=
        ASSERT_EQ(rs->oplogStartHack(opCtx.get(), RecordId(2, 1)), RecordId(1, 2));  // between
        ASSERT_EQ(rs->oplogStartHack(opCtx.get(), RecordId(2, 2)), RecordId(2, 2));  // ==
        ASSERT_EQ(rs->oplogStartHack(opCtx.get(), RecordId(2, 3)), RecordId(2, 2));  // > highest
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper.newOperationContext());
        rs->temp_cappedTruncateAfter(opCtx.get(), RecordId(2, 2), false);  // no-op
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper.newOperationContext());
        ASSERT_EQ(rs->oplogStartHack(opCtx.get(), RecordId(2, 3)), RecordId(2, 2));
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper.newOperationContext());
        rs->temp_cappedTruncateAfter(opCtx.get(), RecordId(1, 2), false);  // deletes 2,2
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper.newOperationContext());
        ASSERT_EQ(rs->oplogStartHack(opCtx.get(), RecordId(2, 3)), RecordId(1, 2));
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper.newOperationContext());
        rs->temp_cappedTruncateAfter(opCtx.get(), RecordId(1, 2), true);  // deletes 1,2
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper.newOperationContext());
        ASSERT_EQ(rs->oplogStartHack(opCtx.get(), RecordId(2, 3)), RecordId(1, 1));
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper.newOperationContext());
        WriteUnitOfWork wuow(opCtx.get());
        ASSERT_OK(rs->truncate(opCtx.get()));  // deletes 1,1 and leaves collection empty
        wuow.commit();
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper.newOperationContext());
        ASSERT_EQ(rs->oplogStartHack(opCtx.get(), RecordId(2, 3)), RecordId());
    }
}

TEST(RocksRecordStoreTest, OplogHackOnNonOplog) {
    RocksRecordStoreHarnessHelper harnessHelper;
    scoped_ptr<RecordStore> rs(harnessHelper.newNonCappedRecordStore("local.NOT_oplog.foo"));

    scoped_ptr<OperationContext> opCtx(harnessHelper.newOperationContext());

    BSONObj obj = BSON("ts" << OpTime(2, -1));
    {
        WriteUnitOfWork wuow(opCtx.get());
        ASSERT_OK(rs->insertRecord(opCtx.get(), obj.objdata(), obj.objsize(), false).getStatus());
        wuow.commit();
    }
    ASSERT_EQ(rs->oplogStartHack(opCtx.get(), RecordId(0, 1)), boost::none);
}

TEST(RocksRecordStoreTest, CappedOrder) {
    scoped_ptr<RocksRecordStoreHarnessHelper> harnessHelper(new RocksRecordStoreHarnessHelper());
    scoped_ptr<RecordStore> rs(harnessHelper->newCappedRecordStore("a.b", 100000, 10000));

    RecordId loc1;

    {  // first insert a document
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            WriteUnitOfWork uow(opCtx.get());
            StatusWith<RecordId> res = rs->insertRecord(opCtx.get(), "a", 2, false);
            ASSERT_OK(res.getStatus());
            loc1 = res.getValue();
            uow.commit();
        }
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        scoped_ptr<RecordIterator> it(rs->getIterator(opCtx.get(), loc1));
        ASSERT(!it->isEOF());
        ASSERT_EQ(loc1, it->getNext());
        ASSERT(it->isEOF());
    }

    {
        // now we insert 2 docs, but commit the 2nd one fiirst
        // we make sure we can't find the 2nd until the first is commited
        scoped_ptr<OperationContext> t1(harnessHelper->newOperationContext());
        scoped_ptr<WriteUnitOfWork> w1(new WriteUnitOfWork(t1.get()));
        rs->insertRecord(t1.get(), "b", 2, false);
        // do not commit yet

        {  // create 2nd doc
            scoped_ptr<OperationContext> t2(harnessHelper->newOperationContext());
            {
                WriteUnitOfWork w2(t2.get());
                rs->insertRecord(t2.get(), "c", 2, false);
                w2.commit();
            }
        }

        {  // state should be the same
            scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
            scoped_ptr<RecordIterator> it(rs->getIterator(opCtx.get(), loc1));
            ASSERT(!it->isEOF());
            ASSERT_EQ(loc1, it->getNext());
            ASSERT(it->isEOF());
        }

        w1->commit();
    }

    {  // now all 3 docs should be visible
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        scoped_ptr<RecordIterator> it(rs->getIterator(opCtx.get(), loc1));
        ASSERT(!it->isEOF());
        ASSERT_EQ(loc1, it->getNext());
        ASSERT(!it->isEOF());
        it->getNext();
        ASSERT(!it->isEOF());
        it->getNext();
        ASSERT(it->isEOF());
    }
}

RecordId _oplogOrderInsertOplog(OperationContext* txn, scoped_ptr<RecordStore>& rs, int inc) {
    OpTime opTime = OpTime(5, inc);
    RocksRecordStore* rrs = dynamic_cast<RocksRecordStore*>(rs.get());
    Status status = rrs->oplogDiskLocRegister(txn, opTime);
    ASSERT_OK(status);
    BSONObj obj = BSON("ts" << opTime);
    StatusWith<RecordId> res = rs->insertRecord(txn, obj.objdata(), obj.objsize(), false);
    ASSERT_OK(res.getStatus());
    return res.getValue();
}

TEST(RocksRecordStoreTest, OplogOrder) {
    scoped_ptr<RocksRecordStoreHarnessHelper> harnessHelper(new RocksRecordStoreHarnessHelper());
    scoped_ptr<RecordStore> rs(
        harnessHelper->newCappedRecordStore("local.oplog.foo", 100000, 10000));
    {
        const RocksRecordStore* rrs = dynamic_cast<RocksRecordStore*>(rs.get());
        ASSERT(rrs->isOplog());
    }

    RecordId loc1;

    {  // first insert a document
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            WriteUnitOfWork uow(opCtx.get());
            loc1 = _oplogOrderInsertOplog(opCtx.get(), rs, 1);
            uow.commit();
        }
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        scoped_ptr<RecordIterator> it(rs->getIterator(opCtx.get(), loc1));
        ASSERT(!it->isEOF());
        ASSERT_EQ(loc1, it->getNext());
        ASSERT(it->isEOF());
    }

    {
        // now we insert 2 docs, but commit the 2nd one fiirst
        // we make sure we can't find the 2nd until the first is commited
        scoped_ptr<OperationContext> t1(harnessHelper->newOperationContext());
        scoped_ptr<WriteUnitOfWork> w1(new WriteUnitOfWork(t1.get()));
        _oplogOrderInsertOplog(t1.get(), rs, 2);
        // do not commit yet

        {  // create 2nd doc
            scoped_ptr<OperationContext> t2(harnessHelper->newOperationContext());
            {
                WriteUnitOfWork w2(t2.get());
                _oplogOrderInsertOplog(t2.get(), rs, 3);
                w2.commit();
            }
        }

        {  // state should be the same
            scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
            scoped_ptr<RecordIterator> it(rs->getIterator(opCtx.get(), loc1));
            ASSERT(!it->isEOF());
            ASSERT_EQ(loc1, it->getNext());
            ASSERT(it->isEOF());
        }

        w1->commit();
    }

    {  // now all 3 docs should be visible
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        scoped_ptr<RecordIterator> it(rs->getIterator(opCtx.get(), loc1));
        ASSERT(!it->isEOF());
        ASSERT_EQ(loc1, it->getNext());
        ASSERT(!it->isEOF());
        it->getNext();
        ASSERT(!it->isEOF());
        it->getNext();
        ASSERT(it->isEOF());
    }
}
}
