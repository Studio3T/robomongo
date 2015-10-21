// record_store_test_validate.cpp

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

#include "mongo/db/storage/record_store_test_validate.h"

#include <boost/scoped_ptr.hpp>

#include "mongo/db/storage/record_store.h"
#include "mongo/db/storage/record_store_test_harness.h"
#include "mongo/unittest/unittest.h"

using boost::scoped_ptr;
using std::string;

namespace mongo {
namespace {

// Verify that calling validate() on an empty collection returns an OK status.
// When either of `full` or `scanData` are false, the ValidateAdaptor
// should not be used.
TEST(RecordStoreTestHarness, ValidateEmpty) {
    scoped_ptr<HarnessHelper> harnessHelper(newHarnessHelper());
    scoped_ptr<RecordStore> rs(harnessHelper->newNonCappedRecordStore());

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQUALS(0, rs->numRecords(opCtx.get()));
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            ValidateAdaptorSpy adaptor;
            ValidateResults results;
            BSONObjBuilder stats;
            ASSERT_OK(rs->validate(opCtx.get(),
                                   false,  // full validate
                                   false,  // scan data
                                   &adaptor,
                                   &results,
                                   &stats));
            ASSERT(results.valid);
            ASSERT(results.errors.empty());
        }
    }
}

// Verify that calling validate() on an empty collection returns an OK status.
// When either of `full` or `scanData` are false, the ValidateAdaptor
// should not be used.
TEST(RecordStoreTestHarness, ValidateEmptyAndScanData) {
    scoped_ptr<HarnessHelper> harnessHelper(newHarnessHelper());
    scoped_ptr<RecordStore> rs(harnessHelper->newNonCappedRecordStore());

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQUALS(0, rs->numRecords(opCtx.get()));
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            ValidateAdaptorSpy adaptor;
            ValidateResults results;
            BSONObjBuilder stats;
            ASSERT_OK(rs->validate(opCtx.get(),
                                   false,  // full validate
                                   true,   // scan data
                                   &adaptor,
                                   &results,
                                   &stats));
            ASSERT(results.valid);
            ASSERT(results.errors.empty());
        }
    }
}

// Verify that calling validate() on an empty collection returns an OK status.
// When either of `full` or `scanData` are false, the ValidateAdaptor
// should not be used.
TEST(RecordStoreTestHarness, FullValidateEmpty) {
    scoped_ptr<HarnessHelper> harnessHelper(newHarnessHelper());
    scoped_ptr<RecordStore> rs(harnessHelper->newNonCappedRecordStore());

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQUALS(0, rs->numRecords(opCtx.get()));
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            ValidateAdaptorSpy adaptor;
            ValidateResults results;
            BSONObjBuilder stats;
            ASSERT_OK(rs->validate(opCtx.get(),
                                   true,   // full validate
                                   false,  // scan data
                                   &adaptor,
                                   &results,
                                   &stats));
            ASSERT(results.valid);
            ASSERT(results.errors.empty());
        }
    }
}

// Verify that calling validate() on an empty collection returns an OK status.
TEST(RecordStoreTestHarness, FullValidateEmptyAndScanData) {
    scoped_ptr<HarnessHelper> harnessHelper(newHarnessHelper());
    scoped_ptr<RecordStore> rs(harnessHelper->newNonCappedRecordStore());

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQUALS(0, rs->numRecords(opCtx.get()));
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            ValidateAdaptorSpy adaptor;
            ValidateResults results;
            BSONObjBuilder stats;
            ASSERT_OK(rs->validate(opCtx.get(),
                                   true,  // full validate
                                   true,  // scan data
                                   &adaptor,
                                   &results,
                                   &stats));
            ASSERT(results.valid);
            ASSERT(results.errors.empty());
        }
    }
}

// Insert multiple records, and verify that calling validate() on a nonempty collection
// returns an OK status. When either of `full` or `scanData` are false, the ValidateAdaptor
// should not be used.
TEST_F(ValidateTest, ValidateNonEmpty) {
    {
        scoped_ptr<OperationContext> opCtx(newOperationContext());
        {
            ValidateAdaptorSpy adaptor;
            ValidateResults results;
            BSONObjBuilder stats;
            ASSERT_OK(getRecordStore().validate(opCtx.get(),
                                                false,  // full validate
                                                false,  // scan data
                                                &adaptor,
                                                &results,
                                                &stats));
            ASSERT(results.valid);
            ASSERT(results.errors.empty());
        }
    }
}

// Insert multiple records, and verify that calling validate() on a nonempty collection
// returns an OK status. When either of `full` or `scanData` are false, the ValidateAdaptor
// should not be used.
TEST_F(ValidateTest, ValidateAndScanDataNonEmpty) {
    {
        scoped_ptr<OperationContext> opCtx(newOperationContext());
        {
            ValidateAdaptorSpy adaptor;
            ValidateResults results;
            BSONObjBuilder stats;
            ASSERT_OK(getRecordStore().validate(opCtx.get(),
                                                false,  // full validate
                                                true,   // scan data
                                                &adaptor,
                                                &results,
                                                &stats));
            ASSERT(results.valid);
            ASSERT(results.errors.empty());
        }
    }
}

// Insert multiple records, and verify that calling validate() on a nonempty collection
// returns an OK status. When either of `full` or `scanData` are false, the ValidateAdaptor
// should not be used.
TEST_F(ValidateTest, FullValidateNonEmpty) {
    {
        scoped_ptr<OperationContext> opCtx(newOperationContext());
        {
            ValidateAdaptorSpy adaptor;
            ValidateResults results;
            BSONObjBuilder stats;
            ASSERT_OK(getRecordStore().validate(opCtx.get(),
                                                true,   // full validate
                                                false,  // scan data
                                                &adaptor,
                                                &results,
                                                &stats));
            ASSERT(results.valid);
            ASSERT(results.errors.empty());
        }
    }
}

// Insert multiple records, and verify that calling validate() on a nonempty collection
// returns an OK status.
TEST_F(ValidateTest, FullValidateNonEmptyAndScanData) {
    {
        scoped_ptr<OperationContext> opCtx(newOperationContext());
        {
            ValidateAdaptorSpy adaptor(getInsertedRecords());
            ValidateResults results;
            BSONObjBuilder stats;
            ASSERT_OK(getRecordStore().validate(opCtx.get(),
                                                true,  // full validate
                                                true,  // scan data
                                                &adaptor,
                                                &results,
                                                &stats));
            ASSERT(adaptor.allValidated());
            ASSERT(results.valid);
            ASSERT(results.errors.empty());
        }
    }
}

}  // namespace
}  // namespace mongo
