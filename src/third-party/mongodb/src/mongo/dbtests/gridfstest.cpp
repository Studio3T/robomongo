/**
 *    Copyright (C) 2012 10gen Inc.
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
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

#include "mongo/platform/basic.h"

#include "mongo/client/gridfs.h"
#include "mongo/db/dbdirectclient.h"
#include "mongo/db/operation_context_impl.h"
#include "mongo/dbtests/dbtests.h"
#include "mongo/util/assert_util.h"

using mongo::DBDirectClient;
using mongo::GridFS;
using mongo::MsgAssertionException;

namespace {

class SetChunkSizeTest {
public:
    virtual void run() {
        OperationContextImpl txn;
        DBDirectClient client(&txn);

        GridFS grid(client, "gridtest");
        grid.setChunkSize(5);

        ASSERT_EQUALS(5U, grid.getChunkSize());
        ASSERT_THROWS(grid.setChunkSize(0), MsgAssertionException);
        ASSERT_EQUALS(5U, grid.getChunkSize());
    }

    virtual ~SetChunkSizeTest() {}
};

class All : public Suite {
public:
    All() : Suite("gridfs") {}

    void setupTests() {
        add<SetChunkSizeTest>();
    }
};

SuiteInstance<All> myall;
}
