// sorted_data_interface_test_touch.cpp

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

#include "mongo/db/storage/sorted_data_interface_test_harness.h"

#include <boost/scoped_ptr.hpp>

#include "mongo/db/storage/sorted_data_interface.h"
#include "mongo/unittest/unittest.h"

namespace mongo {

using boost::scoped_ptr;

// Verify that calling touch() on an empty index returns an OK status.
TEST(SortedDataInterface, TouchEmpty) {
    scoped_ptr<HarnessHelper> harnessHelper(newHarnessHelper());
    scoped_ptr<SortedDataInterface> sorted(harnessHelper->newSortedDataInterface(false));

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        Status status = sorted->touch(opCtx.get());
        ASSERT(status.isOK() || status.code() == ErrorCodes::CommandNotSupported);
    }
}

// Verify that calling touch() on a nonempty index returns an OK status.
TEST(SortedDataInterface, TouchNonEmpty) {
    scoped_ptr<HarnessHelper> harnessHelper(newHarnessHelper());
    scoped_ptr<SortedDataInterface> sorted(harnessHelper->newSortedDataInterface(true));

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT(sorted->isEmpty(opCtx.get()));
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        {
            WriteUnitOfWork uow(opCtx.get());
            ASSERT_OK(sorted->insert(opCtx.get(), key1, loc1, false));
            ASSERT_OK(sorted->insert(opCtx.get(), key2, loc2, false));
            ASSERT_OK(sorted->insert(opCtx.get(), key3, loc3, false));
            uow.commit();
        }
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        ASSERT_EQUALS(3, sorted->numEntries(opCtx.get()));
    }

    {
        scoped_ptr<OperationContext> opCtx(harnessHelper->newOperationContext());
        // XXX does not verify the index was brought into memory
        //     (even if supported by storage engine)
        Status status = sorted->touch(opCtx.get());
        ASSERT(status.isOK() || status.code() == ErrorCodes::CommandNotSupported);
    }
}

}  // namespace mongo
