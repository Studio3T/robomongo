// record_store_test_repairiter.cpp

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

#include "mongo/db/storage/record_store_test_harness.h"

#include <boost/scoped_ptr.hpp>

#include "mongo/db/record_id.h"
#include "mongo/db/storage/record_data.h"
#include "mongo/db/storage/record_store.h"
#include "mongo/unittest/unittest.h"

using boost::scoped_ptr;
using std::set;
using std::string;
using std::stringstream;

namespace mongo {

// Create an iterator for repairing an empty record store.
TEST(RecordStoreTestHarness, GetIteratorForRepairEmpty) {
    scoped_ptr<HarnessHelper> harnessHelper(newHarnessHelper());
    scoped_ptr<RecordStore> rs(harnessHelper->newNonCappedRecordStore());

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQUALS(0, rs->numRecords(opCtx.get()));
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        RecordIterator* it = rs->getIteratorForRepair(opCtx.get());

        // returns NULL if getIteratorForRepair is not supported
        if (it == NULL) {
            return;
        }
        ASSERT(it->isEOF());
        ASSERT_EQUALS(RecordId(), it->curr());
        ASSERT_EQUALS(RecordId(), it->getNext());
        ASSERT(it->isEOF());
        ASSERT_EQUALS(RecordId(), it->curr());

        delete it;
    }
}

// Insert multiple records and create an iterator for repairing the record store,
// even though the it has not been corrupted.
TEST(RecordStoreTestHarness, GetIteratorForRepairNonEmpty) {
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

    set<RecordId> remain(locs, locs + nToInsert);
    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        RecordIterator* it = rs->getIteratorForRepair(opCtx.get());
        // returns NULL if getIteratorForRepair is not supported
        if (it == NULL) {
            return;
        }

        while (!it->isEOF()) {
            RecordId loc = it->getNext();
            remain.erase(loc);  // can happen more than once per doc
        }
        ASSERT(remain.empty());

        ASSERT_EQUALS(RecordId(), it->curr());
        ASSERT_EQUALS(RecordId(), it->getNext());
        ASSERT(it->isEOF());
        ASSERT_EQUALS(RecordId(), it->curr());

        delete it;
    }
}

// Insert a single record. Create a repair iterator pointing to that single record.
// Then invalidate the record and ensure that the repair iterator responds correctly.
// See SERVER-16300.
TEST(RecordStoreTestHarness, GetIteratorForRepairInvalidateSingleton) {
    scoped_ptr<HarnessHelper> harnessHelper(newHarnessHelper());
    scoped_ptr<RecordStore> rs(harnessHelper->newNonCappedRecordStore());

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQ(0, rs->numRecords(opCtx.get()));
    }

    // Insert one record.
    RecordId idToInvalidate;
    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        WriteUnitOfWork uow(opCtx.get());
        StatusWith<RecordId> res = rs->insertRecord(opCtx.get(), "some data", 10, false);
        ASSERT_OK(res.getStatus());
        idToInvalidate = res.getValue();
        uow.commit();
    }

    // Double-check that the record store has one record in it now.
    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQ(1, rs->numRecords(opCtx.get()));
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        scoped_ptr<RecordIterator> it(rs->getIteratorForRepair(opCtx.get()));
        // Return value of NULL is expected if getIteratorForRepair is not supported.
        if (!it) {
            return;
        }

        // We should be pointing at the only record in the store.
        ASSERT_EQ(idToInvalidate, it->curr());
        ASSERT(!it->isEOF());

        // Invalidate the record we're pointing at.
        it->saveState();
        it->invalidate(idToInvalidate);
        it->restoreState(opCtx.get());

        // Iterator should be EOF now because the only thing in the collection got deleted.
        ASSERT(it->isEOF());
        ASSERT_EQ(it->getNext(), RecordId());
    }
}

}  // namespace mongo
