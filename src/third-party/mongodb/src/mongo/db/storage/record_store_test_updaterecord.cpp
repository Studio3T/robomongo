// record_store_test_updaterecord.cpp

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

#include "mongo/db/storage/record_store_test_updaterecord.h"

#include <boost/scoped_ptr.hpp>

#include "mongo/db/record_id.h"
#include "mongo/db/storage/record_data.h"
#include "mongo/db/storage/record_store.h"
#include "mongo/db/storage/record_store_test_harness.h"
#include "mongo/unittest/unittest.h"

using boost::scoped_ptr;
using std::string;
using std::stringstream;

namespace mongo {

// Insert a record and try to update it.
TEST(RecordStoreTestHarness, UpdateRecord) {
    scoped_ptr<HarnessHelper> harnessHelper(newHarnessHelper());
    scoped_ptr<RecordStore> rs(harnessHelper->newNonCappedRecordStore());

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQUALS(0, rs->numRecords(opCtx.get()));
    }

    string data = "my record";
    RecordId loc;
    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            WriteUnitOfWork uow(opCtx.get());
            StatusWith<RecordId> res =
                rs->insertRecord(opCtx.get(), data.c_str(), data.size() + 1, false);
            ASSERT_OK(res.getStatus());
            loc = res.getValue();
            uow.commit();
        }
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQUALS(1, rs->numRecords(opCtx.get()));
    }

    data = "updated record-";
    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            WriteUnitOfWork uow(opCtx.get());
            StatusWith<RecordId> res =
                rs->updateRecord(opCtx.get(), loc, data.c_str(), data.size() + 1, false, NULL);
            ASSERT_OK(res.getStatus());
            loc = res.getValue();
            uow.commit();
        }
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            RecordData record = rs->dataFor(opCtx.get(), loc);
            ASSERT_EQUALS(data.size() + 1, static_cast<size_t>(record.size()));
            ASSERT_EQUALS(data, record.data());
        }
    }
}

// Insert multiple records and try to update them.
TEST(RecordStoreTestHarness, UpdateMultipleRecords) {
    scoped_ptr<HarnessHelper> harnessHelper(newHarnessHelper());
    scoped_ptr<RecordStore> rs(harnessHelper->newNonCappedRecordStore());

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQUALS(0, rs->numRecords(opCtx.get()));
    }

    const int nToInsert = 10;
    RecordId locs[nToInsert];
    for (int i = 0; i < nToInsert; i++) {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            stringstream ss;
            ss << "record " << i;
            string data = ss.str();

            WriteUnitOfWork uow(opCtx.get());
            StatusWith<RecordId> res =
                rs->insertRecord(opCtx.get(), data.c_str(), data.size() + 1, false);
            ASSERT_OK(res.getStatus());
            locs[i] = res.getValue();
            uow.commit();
        }
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQUALS(nToInsert, rs->numRecords(opCtx.get()));
    }

    for (int i = 0; i < nToInsert; i++) {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            stringstream ss;
            ss << "update record-" << i;
            string data = ss.str();

            WriteUnitOfWork uow(opCtx.get());
            StatusWith<RecordId> res =
                rs->updateRecord(opCtx.get(), locs[i], data.c_str(), data.size() + 1, false, NULL);
            ASSERT_OK(res.getStatus());
            locs[i] = res.getValue();
            uow.commit();
        }
    }

    for (int i = 0; i < nToInsert; i++) {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            stringstream ss;
            ss << "update record-" << i;
            string data = ss.str();

            RecordData record = rs->dataFor(opCtx.get(), locs[i]);
            ASSERT_EQUALS(data.size() + 1, static_cast<size_t>(record.size()));
            ASSERT_EQUALS(data, record.data());
        }
    }
}

// Insert a record, try to update it, and examine how the UpdateNotifier is called.
TEST(RecordStoreTestHarness, UpdateRecordWithMoveNotifier) {
    scoped_ptr<HarnessHelper> harnessHelper(newHarnessHelper());
    scoped_ptr<RecordStore> rs(harnessHelper->newNonCappedRecordStore());

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQUALS(0, rs->numRecords(opCtx.get()));
    }

    string oldData = "my record";
    RecordId loc;
    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            WriteUnitOfWork uow(opCtx.get());
            StatusWith<RecordId> res =
                rs->insertRecord(opCtx.get(), oldData.c_str(), oldData.size() + 1, false);
            ASSERT_OK(res.getStatus());
            loc = res.getValue();
            uow.commit();
        }
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQUALS(1, rs->numRecords(opCtx.get()));
    }

    string newData = "my updated record--";
    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            UpdateNotifierSpy umn(opCtx.get(), loc, oldData.c_str(), oldData.size());

            WriteUnitOfWork uow(opCtx.get());
            StatusWith<RecordId> res = rs->updateRecord(
                opCtx.get(), loc, newData.c_str(), newData.size() + 1, false, &umn);
            ASSERT_OK(res.getStatus());
            // UpdateNotifier::recordStoreGoingToMove() called only if
            // the RecordId for the record changes
            if (loc == res.getValue()) {
                ASSERT_EQUALS(0, umn.numMoveCallbacks());
                // Only MMAP v1 is required to use the UpdateNotifier for in-place updates,
                // so the number of callbacks is expected to be 0 for non-MMAP storage engines.
                ASSERT_GTE(1, umn.numInPlaceCallbacks());
            } else {
                ASSERT_EQUALS(1, umn.numMoveCallbacks());
                ASSERT_EQUALS(0, umn.numInPlaceCallbacks());
            }
            loc = res.getValue();
            uow.commit();
        }
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            RecordData record = rs->dataFor(opCtx.get(), loc);
            ASSERT_EQUALS(newData.size() + 1, static_cast<size_t>(record.size()));
            ASSERT_EQUALS(newData, record.data());
        }
    }
}

}  // namespace mongo
