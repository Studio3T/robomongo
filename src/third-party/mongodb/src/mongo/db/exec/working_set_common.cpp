/**
 *    Copyright (C) 2013 10gen Inc.
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

#include "mongo/db/catalog/collection.h"
#include "mongo/db/global_environment_experiment.h"
#include "mongo/db/exec/working_set.h"
#include "mongo/db/exec/working_set_common.h"

namespace mongo {

// static
bool WorkingSetCommon::fetchAndInvalidateLoc(OperationContext* txn,
                                             WorkingSetMember* member,
                                             const Collection* collection) {
    // Already in our desired state.
    if (member->state == WorkingSetMember::OWNED_OBJ) {
        return true;
    }

    // We can't do anything without a RecordId.
    if (!member->hasLoc()) {
        return false;
    }

    // Do the fetch, invalidate the DL.
    member->obj = collection->docFor(txn, member->loc);
    member->obj.setValue(member->obj.value().getOwned());

    member->state = WorkingSetMember::OWNED_OBJ;
    member->loc = RecordId();
    return true;
}

// static
void WorkingSetCommon::forceFetchAllLocs(OperationContext* txn,
                                         WorkingSet* workingSet,
                                         const Collection* collection) {
    invariant(collection);
    dassert(supportsDocLocking());

    for (auto id : workingSet->getAndClearIdxIds()) {
        if (workingSet->isFree(id)) {
            continue;
        }

        WorkingSetMember* member = workingSet->get(id);
        if (WorkingSetMember::LOC_AND_IDX != member->state) {
            continue;
        }

        // Do the fetch. It is possible in normal operation for the object keyed by this
        // member's RecordId to no longer be present in the collection. Consider the case of a
        // delete operation with three possible plans. During the course of plan selection,
        // each candidate plan creates a working set member for document D. Then plan P wins,
        // and starts to delete the matching documents, including D. The working set members for
        // D created by the two rejected are still present, but their RecordIds no longer refer
        // to a valid document.
        member->obj.reset();
        if (!collection->findDoc(txn, member->loc, &member->obj)) {
            // Leftover working set members pointing to old docs can be safely freed.
            workingSet->free(id);
            continue;
        }

        // We rely on the assumption that doc-locking storage engines always return owned BSON.
        // This assumption may become invalid in future versions but must remain valid on the
        // 3.0 branch.
        invariant(member->obj.value().isOwned());

        member->keyData.clear();
        member->state = WorkingSetMember::LOC_AND_OBJ;
    }
}

// static
void WorkingSetCommon::completeFetch(OperationContext* txn,
                                     WorkingSetMember* member,
                                     const Collection* collection) {
    // The RecordFetcher should already have been transferred out of the WSM and used.
    invariant(!member->hasFetcher());

    // If the diskloc was invalidated during fetch, then a "forced fetch" already converted this
    // WSM into the owned object state. In this case, there is nothing more to do here.
    if (member->hasOwnedObj()) {
        return;
    }

    // We should have a RecordId but need to retrieve the obj. Get the obj now and reset all WSM
    // state appropriately.
    invariant(member->hasLoc());
    member->obj = collection->docFor(txn, member->loc);
    member->keyData.clear();
    member->state = WorkingSetMember::LOC_AND_OBJ;
}

// static
void WorkingSetCommon::initFrom(WorkingSetMember* dest, const WorkingSetMember& src) {
    dest->loc = src.loc;
    dest->obj = src.obj;
    dest->keyData = src.keyData;
    dest->state = src.state;

    // Merge computed data.
    typedef WorkingSetComputedDataType WSCD;
    for (WSCD i = WSCD(0); i < WSM_COMPUTED_NUM_TYPES; i = WSCD(i + 1)) {
        if (src.hasComputed(i)) {
            dest->addComputed(src.getComputed(i)->clone());
        }
    }
}

// static
BSONObj WorkingSetCommon::buildMemberStatusObject(const Status& status) {
    BSONObjBuilder bob;
    bob.append("ok", status.isOK() ? 1.0 : 0.0);
    bob.append("code", status.code());
    bob.append("errmsg", status.reason());

    return bob.obj();
}

// static
WorkingSetID WorkingSetCommon::allocateStatusMember(WorkingSet* ws, const Status& status) {
    invariant(ws);

    WorkingSetID wsid = ws->allocate();
    WorkingSetMember* member = ws->get(wsid);
    member->state = WorkingSetMember::OWNED_OBJ;
    member->obj = Snapshotted<BSONObj>(SnapshotId(), buildMemberStatusObject(status));

    return wsid;
}

// static
bool WorkingSetCommon::isValidStatusMemberObject(const BSONObj& obj) {
    return obj.nFields() == 3 && obj.hasField("ok") && obj.hasField("code") &&
        obj.hasField("errmsg");
}

// static
void WorkingSetCommon::getStatusMemberObject(const WorkingSet& ws,
                                             WorkingSetID wsid,
                                             BSONObj* objOut) {
    invariant(objOut);

    // Validate ID and working set member.
    if (WorkingSet::INVALID_ID == wsid) {
        return;
    }
    WorkingSetMember* member = ws.get(wsid);
    if (!member->hasOwnedObj()) {
        return;
    }
    BSONObj obj = member->obj.value();
    if (!isValidStatusMemberObject(obj)) {
        return;
    }
    *objOut = obj;
}

// static
Status WorkingSetCommon::getMemberObjectStatus(const BSONObj& memberObj) {
    invariant(WorkingSetCommon::isValidStatusMemberObject(memberObj));
    return Status(static_cast<ErrorCodes::Error>(memberObj["code"].numberInt()),
                  memberObj["errmsg"]);
}

// static
Status WorkingSetCommon::getMemberStatus(const WorkingSetMember& member) {
    invariant(member.hasObj());
    return getMemberObjectStatus(member.obj.value());
}

// static
std::string WorkingSetCommon::toStatusString(const BSONObj& obj) {
    if (!isValidStatusMemberObject(obj)) {
        Status unknownStatus(ErrorCodes::UnknownError, "no details available");
        return unknownStatus.toString();
    }
    Status status(ErrorCodes::fromInt(obj.getIntField("code")), obj.getStringField("errmsg"));
    return status.toString();
}

}  // namespace mongo
