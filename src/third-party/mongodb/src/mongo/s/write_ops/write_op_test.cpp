/**
 *    Copyright (C) 2013 MongoDB Inc.
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

#include "mongo/s/write_ops/write_op.h"

#include <boost/scoped_ptr.hpp>

#include "mongo/base/error_codes.h"
#include "mongo/base/owned_pointer_vector.h"
#include "mongo/s/mock_ns_targeter.h"
#include "mongo/s/write_ops/batched_command_request.h"
#include "mongo/s/write_ops/batched_delete_document.h"
#include "mongo/s/write_ops/write_error_detail.h"
#include "mongo/unittest/unittest.h"

namespace {

using boost::scoped_ptr;
using std::string;
using std::vector;

using namespace mongo;

WriteErrorDetail* buildError(int code, const BSONObj& info, const string& message) {
    WriteErrorDetail* error = new WriteErrorDetail();
    error->setErrCode(code);
    error->setErrInfo(info);
    error->setErrMessage(message);

    return error;
}

TEST(WriteOpTests, BasicError) {
    //
    // Test of basic error-setting on write op
    //

    BatchedCommandRequest request(BatchedCommandRequest::BatchType_Insert);
    request.setNS("foo.bar");
    request.getInsertRequest()->addToDocuments(BSON("x" << 1));

    WriteOp writeOp(BatchItemRef(&request, 0));
    ASSERT_EQUALS(writeOp.getWriteState(), WriteOpState_Ready);

    scoped_ptr<WriteErrorDetail> error(
        buildError(ErrorCodes::UnknownError, BSON("data" << 12345), "some message"));

    writeOp.setOpError(*error);
    ASSERT_EQUALS(writeOp.getWriteState(), WriteOpState_Error);
    ASSERT_EQUALS(writeOp.getOpError().getErrCode(), error->getErrCode());
    ASSERT_EQUALS(writeOp.getOpError().getErrInfo()["data"].Int(),
                  error->getErrInfo()["data"].Int());
    ASSERT_EQUALS(writeOp.getOpError().getErrMessage(), error->getErrMessage());
}

TEST(WriteOpTests, TargetSingle) {
    //
    // Basic targeting test
    //

    NamespaceString nss("foo.bar");

    ShardEndpoint endpoint("shard", ChunkVersion::IGNORED());

    vector<MockRange*> mockRanges;
    mockRanges.push_back(new MockRange(endpoint, nss, BSON("x" << MINKEY), BSON("x" << MAXKEY)));

    BatchedCommandRequest request(BatchedCommandRequest::BatchType_Insert);
    request.setNS(nss.ns());
    request.getInsertRequest()->addToDocuments(BSON("x" << 1));

    // Do single-target write op

    WriteOp writeOp(BatchItemRef(&request, 0));
    ASSERT_EQUALS(writeOp.getWriteState(), WriteOpState_Ready);

    MockNSTargeter targeter;
    targeter.init(mockRanges);

    OwnedPointerVector<TargetedWrite> targetedOwned;
    vector<TargetedWrite*>& targeted = targetedOwned.mutableVector();
    Status status = writeOp.targetWrites(targeter, &targeted);

    ASSERT(status.isOK());
    ASSERT_EQUALS(writeOp.getWriteState(), WriteOpState_Pending);
    ASSERT_EQUALS(targeted.size(), 1u);
    assertEndpointsEqual(targeted.front()->endpoint, endpoint);

    writeOp.noteWriteComplete(*targeted.front());
    ASSERT_EQUALS(writeOp.getWriteState(), WriteOpState_Completed);
}

BatchedDeleteDocument* buildDeleteDoc(const BSONObj& doc) {
    BatchedDeleteDocument* deleteDoc = new BatchedDeleteDocument();

    string errMsg;
    bool ok = deleteDoc->parseBSON(doc, &errMsg);
    ASSERT_EQUALS(errMsg, "");
    ASSERT(ok);
    return deleteDoc;
}

struct EndpointComp {
    bool operator()(const TargetedWrite* writeA, const TargetedWrite* writeB) const {
        return writeA->endpoint.shardName.compare(writeB->endpoint.shardName) < 0;
    }
};

inline void sortByEndpoint(vector<TargetedWrite*>* writes) {
    std::sort(writes->begin(), writes->end(), EndpointComp());
}

TEST(WriteOpTests, TargetMultiOneShard) {
    //
    // Multi-write targeting test where our query goes to one shard
    //

    NamespaceString nss("foo.bar");

    ShardEndpoint endpointA("shardA", ChunkVersion(10, 0, OID()));
    ShardEndpoint endpointB("shardB", ChunkVersion(20, 0, OID()));
    ShardEndpoint endpointC("shardB", ChunkVersion(20, 0, OID()));

    vector<MockRange*> mockRanges;
    mockRanges.push_back(new MockRange(endpointA, nss, BSON("x" << MINKEY), BSON("x" << 0)));
    mockRanges.push_back(new MockRange(endpointB, nss, BSON("x" << 0), BSON("x" << 10)));
    mockRanges.push_back(new MockRange(endpointC, nss, BSON("x" << 10), BSON("x" << MAXKEY)));

    BatchedCommandRequest request(BatchedCommandRequest::BatchType_Delete);
    request.setNS(nss.ns());
    // Only hits first shard
    BSONObj query = BSON("x" << GTE << -2 << LT << -1);
    request.getDeleteRequest()->addToDeletes(buildDeleteDoc(BSON("q" << query)));

    WriteOp writeOp(BatchItemRef(&request, 0));
    ASSERT_EQUALS(writeOp.getWriteState(), WriteOpState_Ready);

    MockNSTargeter targeter;
    targeter.init(mockRanges);

    OwnedPointerVector<TargetedWrite> targetedOwned;
    vector<TargetedWrite*>& targeted = targetedOwned.mutableVector();
    Status status = writeOp.targetWrites(targeter, &targeted);

    ASSERT(status.isOK());
    ASSERT_EQUALS(writeOp.getWriteState(), WriteOpState_Pending);
    ASSERT_EQUALS(targeted.size(), 1u);
    assertEndpointsEqual(targeted.front()->endpoint, endpointA);

    writeOp.noteWriteComplete(*targeted.front());

    ASSERT_EQUALS(writeOp.getWriteState(), WriteOpState_Completed);
}

TEST(WriteOpTests, TargetMultiAllShards) {
    //
    // Multi-write targeting test where our write goes to more than one shard
    //

    NamespaceString nss("foo.bar");

    ShardEndpoint endpointA("shardA", ChunkVersion(10, 0, OID()));
    ShardEndpoint endpointB("shardB", ChunkVersion(20, 0, OID()));
    ShardEndpoint endpointC("shardB", ChunkVersion(20, 0, OID()));

    vector<MockRange*> mockRanges;
    mockRanges.push_back(new MockRange(endpointA, nss, BSON("x" << MINKEY), BSON("x" << 0)));
    mockRanges.push_back(new MockRange(endpointB, nss, BSON("x" << 0), BSON("x" << 10)));
    mockRanges.push_back(new MockRange(endpointC, nss, BSON("x" << 10), BSON("x" << MAXKEY)));

    BatchedCommandRequest request(BatchedCommandRequest::BatchType_Delete);
    request.setNS(nss.ns());
    BSONObj query = BSON("x" << GTE << -1 << LT << 1);
    request.getDeleteRequest()->addToDeletes(buildDeleteDoc(BSON("q" << query)));

    // Do multi-target write op

    WriteOp writeOp(BatchItemRef(&request, 0));
    ASSERT_EQUALS(writeOp.getWriteState(), WriteOpState_Ready);

    MockNSTargeter targeter;
    targeter.init(mockRanges);

    OwnedPointerVector<TargetedWrite> targetedOwned;
    vector<TargetedWrite*>& targeted = targetedOwned.mutableVector();
    Status status = writeOp.targetWrites(targeter, &targeted);

    ASSERT(status.isOK());
    ASSERT_EQUALS(writeOp.getWriteState(), WriteOpState_Pending);
    ASSERT_EQUALS(targeted.size(), 3u);
    sortByEndpoint(&targeted);
    ASSERT_EQUALS(targeted[0]->endpoint.shardName, endpointA.shardName);
    ASSERT(ChunkVersion::isIgnoredVersion(targeted[0]->endpoint.shardVersion));
    ASSERT_EQUALS(targeted[1]->endpoint.shardName, endpointB.shardName);
    ASSERT(ChunkVersion::isIgnoredVersion(targeted[1]->endpoint.shardVersion));
    ASSERT_EQUALS(targeted[2]->endpoint.shardName, endpointC.shardName);
    ASSERT(ChunkVersion::isIgnoredVersion(targeted[2]->endpoint.shardVersion));

    writeOp.noteWriteComplete(*targeted[0]);
    writeOp.noteWriteComplete(*targeted[1]);
    writeOp.noteWriteComplete(*targeted[2]);

    ASSERT_EQUALS(writeOp.getWriteState(), WriteOpState_Completed);
}

TEST(WriteOpTests, ErrorSingle) {
    //
    // Single error after targeting test
    //

    NamespaceString nss("foo.bar");

    ShardEndpoint endpoint("shard", ChunkVersion::IGNORED());

    vector<MockRange*> mockRanges;
    mockRanges.push_back(new MockRange(endpoint, nss, BSON("x" << MINKEY), BSON("x" << MAXKEY)));

    BatchedCommandRequest request(BatchedCommandRequest::BatchType_Insert);
    request.setNS(nss.ns());
    request.getInsertRequest()->addToDocuments(BSON("x" << 1));

    // Do single-target write op

    WriteOp writeOp(BatchItemRef(&request, 0));
    ASSERT_EQUALS(writeOp.getWriteState(), WriteOpState_Ready);

    MockNSTargeter targeter;
    targeter.init(mockRanges);

    OwnedPointerVector<TargetedWrite> targetedOwned;
    vector<TargetedWrite*>& targeted = targetedOwned.mutableVector();
    Status status = writeOp.targetWrites(targeter, &targeted);

    ASSERT(status.isOK());
    ASSERT_EQUALS(writeOp.getWriteState(), WriteOpState_Pending);
    ASSERT_EQUALS(targeted.size(), 1u);
    assertEndpointsEqual(targeted.front()->endpoint, endpoint);

    scoped_ptr<WriteErrorDetail> error(
        buildError(ErrorCodes::UnknownError, BSON("data" << 12345), "some message"));

    writeOp.noteWriteError(*targeted.front(), *error);

    ASSERT_EQUALS(writeOp.getWriteState(), WriteOpState_Error);
    ASSERT_EQUALS(writeOp.getOpError().getErrCode(), error->getErrCode());
    ASSERT_EQUALS(writeOp.getOpError().getErrInfo()["data"].Int(),
                  error->getErrInfo()["data"].Int());
    ASSERT_EQUALS(writeOp.getOpError().getErrMessage(), error->getErrMessage());
}

TEST(WriteOpTests, CancelSingle) {
    //
    // Cancel single targeting test
    //

    NamespaceString nss("foo.bar");

    ShardEndpoint endpoint("shard", ChunkVersion::IGNORED());

    vector<MockRange*> mockRanges;
    mockRanges.push_back(new MockRange(endpoint, nss, BSON("x" << MINKEY), BSON("x" << MAXKEY)));

    BatchedCommandRequest request(BatchedCommandRequest::BatchType_Insert);
    request.setNS(nss.ns());
    request.getInsertRequest()->addToDocuments(BSON("x" << 1));

    // Do single-target write op

    WriteOp writeOp(BatchItemRef(&request, 0));
    ASSERT_EQUALS(writeOp.getWriteState(), WriteOpState_Ready);

    MockNSTargeter targeter;
    targeter.init(mockRanges);

    OwnedPointerVector<TargetedWrite> targetedOwned;
    vector<TargetedWrite*>& targeted = targetedOwned.mutableVector();
    Status status = writeOp.targetWrites(targeter, &targeted);

    ASSERT(status.isOK());
    ASSERT_EQUALS(writeOp.getWriteState(), WriteOpState_Pending);
    ASSERT_EQUALS(targeted.size(), 1u);
    assertEndpointsEqual(targeted.front()->endpoint, endpoint);

    writeOp.cancelWrites(NULL);

    ASSERT_EQUALS(writeOp.getWriteState(), WriteOpState_Ready);
}

//
// Test retryable errors
//

TEST(WriteOpTests, RetrySingleOp) {
    //
    // Retry single targeting test
    //

    NamespaceString nss("foo.bar");

    ShardEndpoint endpoint("shard", ChunkVersion::IGNORED());

    vector<MockRange*> mockRanges;
    mockRanges.push_back(new MockRange(endpoint, nss, BSON("x" << MINKEY), BSON("x" << MAXKEY)));

    BatchedCommandRequest request(BatchedCommandRequest::BatchType_Insert);
    request.setNS(nss.ns());
    request.getInsertRequest()->addToDocuments(BSON("x" << 1));

    // Do single-target write op

    WriteOp writeOp(BatchItemRef(&request, 0));
    ASSERT_EQUALS(writeOp.getWriteState(), WriteOpState_Ready);

    MockNSTargeter targeter;
    targeter.init(mockRanges);

    OwnedPointerVector<TargetedWrite> targetedOwned;
    vector<TargetedWrite*>& targeted = targetedOwned.mutableVector();
    Status status = writeOp.targetWrites(targeter, &targeted);

    ASSERT(status.isOK());
    ASSERT_EQUALS(writeOp.getWriteState(), WriteOpState_Pending);
    ASSERT_EQUALS(targeted.size(), 1u);
    assertEndpointsEqual(targeted.front()->endpoint, endpoint);

    // Stale exception

    scoped_ptr<WriteErrorDetail> error(
        buildError(ErrorCodes::StaleShardVersion, BSON("data" << 12345), "some message"));

    writeOp.noteWriteError(*targeted.front(), *error);

    ASSERT_EQUALS(writeOp.getWriteState(), WriteOpState_Ready);
}

}  // unnamed namespace
