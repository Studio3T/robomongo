// record_store_test_updatewithdamages.cpp

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
using std::string;

namespace mongo {

// Insert a record and try to perform an in-place update on it.
TEST(RecordStoreTestHarness, UpdateWithDamages) {
    scoped_ptr<HarnessHelper> harnessHelper(newHarnessHelper());
    scoped_ptr<RecordStore> rs(harnessHelper->newNonCappedRecordStore());

    if (!rs->updateWithDamagesSupported())
        return;

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQUALS(0, rs->numRecords(opCtx.get()));
    }

    string data = "00010111";
    RecordId loc;
    const RecordData rec(data.c_str(), data.size() + 1);
    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            WriteUnitOfWork uow(opCtx.get());
            StatusWith<RecordId> res = rs->insertRecord(opCtx.get(), rec.data(), rec.size(), false);
            ASSERT_OK(res.getStatus());
            loc = res.getValue();
            uow.commit();
        }
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQUALS(1, rs->numRecords(opCtx.get()));
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            mutablebson::DamageVector dv(3);
            dv[0].sourceOffset = 5;
            dv[0].targetOffset = 0;
            dv[0].size = 2;
            dv[1].sourceOffset = 3;
            dv[1].targetOffset = 2;
            dv[1].size = 3;
            dv[2].sourceOffset = 0;
            dv[2].targetOffset = 5;
            dv[2].size = 3;

            WriteUnitOfWork uow(opCtx.get());
            ASSERT_OK(rs->updateWithDamages(opCtx.get(), loc, rec, data.c_str(), dv));
            uow.commit();
        }
    }

    data = "11101000";
    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            RecordData record = rs->dataFor(opCtx.get(), loc);
            ASSERT_EQUALS(data, record.data());
        }
    }
}

// Insert a record and try to perform an in-place update on it with a DamageVector
// containing overlapping DamageEvents.
TEST(RecordStoreTestHarness, UpdateWithOverlappingDamageEvents) {
    scoped_ptr<HarnessHelper> harnessHelper(newHarnessHelper());
    scoped_ptr<RecordStore> rs(harnessHelper->newNonCappedRecordStore());

    if (!rs->updateWithDamagesSupported())
        return;

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQUALS(0, rs->numRecords(opCtx.get()));
    }

    string data = "00010111";
    RecordId loc;
    const RecordData rec(data.c_str(), data.size() + 1);
    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            WriteUnitOfWork uow(opCtx.get());
            StatusWith<RecordId> res = rs->insertRecord(opCtx.get(), rec.data(), rec.size(), false);
            ASSERT_OK(res.getStatus());
            loc = res.getValue();
            uow.commit();
        }
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQUALS(1, rs->numRecords(opCtx.get()));
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            mutablebson::DamageVector dv(2);
            dv[0].sourceOffset = 3;
            dv[0].targetOffset = 0;
            dv[0].size = 5;
            dv[1].sourceOffset = 0;
            dv[1].targetOffset = 3;
            dv[1].size = 5;

            WriteUnitOfWork uow(opCtx.get());
            ASSERT_OK(rs->updateWithDamages(opCtx.get(), loc, rec, data.c_str(), dv));
            uow.commit();
        }
    }

    data = "10100010";
    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            RecordData record = rs->dataFor(opCtx.get(), loc);
            ASSERT_EQUALS(data, record.data());
        }
    }
}

// Insert a record and try to perform an in-place update on it with a DamageVector
// containing overlapping DamageEvents. The changes should be applied in the order
// specified by the DamageVector, and not -- for instance -- by the targetOffset.
TEST(RecordStoreTestHarness, UpdateWithOverlappingDamageEventsReversed) {
    scoped_ptr<HarnessHelper> harnessHelper(newHarnessHelper());
    scoped_ptr<RecordStore> rs(harnessHelper->newNonCappedRecordStore());

    if (!rs->updateWithDamagesSupported())
        return;

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQUALS(0, rs->numRecords(opCtx.get()));
    }

    string data = "00010111";
    RecordId loc;
    const RecordData rec(data.c_str(), data.size() + 1);
    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            WriteUnitOfWork uow(opCtx.get());
            StatusWith<RecordId> res = rs->insertRecord(opCtx.get(), rec.data(), rec.size(), false);
            ASSERT_OK(res.getStatus());
            loc = res.getValue();
            uow.commit();
        }
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQUALS(1, rs->numRecords(opCtx.get()));
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            mutablebson::DamageVector dv(2);
            dv[0].sourceOffset = 0;
            dv[0].targetOffset = 3;
            dv[0].size = 5;
            dv[1].sourceOffset = 3;
            dv[1].targetOffset = 0;
            dv[1].size = 5;

            WriteUnitOfWork uow(opCtx.get());
            ASSERT_OK(rs->updateWithDamages(opCtx.get(), loc, rec, data.c_str(), dv));
            uow.commit();
        }
    }

    data = "10111010";
    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            RecordData record = rs->dataFor(opCtx.get(), loc);
            ASSERT_EQUALS(data, record.data());
        }
    }
}

// Insert a record and try to call updateWithDamages() with an empty DamageVector.
TEST(RecordStoreTestHarness, UpdateWithNoDamages) {
    scoped_ptr<HarnessHelper> harnessHelper(newHarnessHelper());
    scoped_ptr<RecordStore> rs(harnessHelper->newNonCappedRecordStore());

    if (!rs->updateWithDamagesSupported())
        return;

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQUALS(0, rs->numRecords(opCtx.get()));
    }

    string data = "my record";
    RecordId loc;
    const RecordData rec(data.c_str(), data.size() + 1);
    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            WriteUnitOfWork uow(opCtx.get());
            StatusWith<RecordId> res = rs->insertRecord(opCtx.get(), rec.data(), rec.size(), false);
            ASSERT_OK(res.getStatus());
            loc = res.getValue();
            uow.commit();
        }
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQUALS(1, rs->numRecords(opCtx.get()));
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            mutablebson::DamageVector dv;

            WriteUnitOfWork uow(opCtx.get());
            ASSERT_OK(rs->updateWithDamages(opCtx.get(), loc, rec, "", dv));
            uow.commit();
        }
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            RecordData record = rs->dataFor(opCtx.get(), loc);
            ASSERT_EQUALS(data, record.data());
        }
    }
}

}  // namespace mongo
