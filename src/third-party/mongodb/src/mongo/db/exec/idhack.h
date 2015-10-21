/**
 *    Copyright (C) 2013-2014 MongoDB Inc.
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

#pragma once

#include "mongo/db/catalog/collection.h"
#include "mongo/db/exec/plan_stage.h"
#include "mongo/db/query/canonical_query.h"
#include "mongo/db/record_id.h"

namespace mongo {

/**
 * A standalone stage implementing the fast path for key-value retrievals
 * via the _id index.
 */
class IDHackStage : public PlanStage {
public:
    /** Takes ownership of all the arguments -collection. */
    IDHackStage(OperationContext* txn,
                const Collection* collection,
                CanonicalQuery* query,
                WorkingSet* ws);

    IDHackStage(OperationContext* txn, Collection* collection, const BSONObj& key, WorkingSet* ws);

    virtual ~IDHackStage();

    virtual bool isEOF();
    virtual StageState work(WorkingSetID* out);

    virtual void saveState();
    virtual void restoreState(OperationContext* opCtx);
    virtual void invalidate(OperationContext* txn, const RecordId& dl, InvalidationType type);

    /**
     * ID Hack has a very strict criteria for the queries it supports.
     */
    static bool supportsQuery(const CanonicalQuery& query);

    virtual std::vector<PlanStage*> getChildren() const;

    virtual StageType stageType() const {
        return STAGE_IDHACK;
    }

    PlanStageStats* getStats();

    virtual const CommonStats* getCommonStats();

    virtual const SpecificStats* getSpecificStats();

    static const char* kStageType;

private:
    /**
     * Marks this stage as done, optionally adds key metadata, and returns PlanStage::ADVANCED.
     *
     * Called whenever we have a WSM containing the matching obj.
     */
    StageState advance(WorkingSetID id, WorkingSetMember* member, WorkingSetID* out);

    // transactional context for read locks. Not owned by us
    OperationContext* _txn;

    // Not owned here.
    const Collection* _collection;

    // The WorkingSet we annotate with results.  Not owned by us.
    WorkingSet* _workingSet;

    // The value to match against the _id field.
    BSONObj _key;

    // Have we returned our one document?
    bool _done;

    // Do we need to add index key metadata for $returnKey?
    bool _addKeyMetadata;

    // If we want to return a RecordId and it points to something that's not in memory,
    // we return a "please page this in" result. We add a RecordFetcher given back to us by the
    // storage engine to the WSM. The RecordFetcher is used by the PlanExecutor when it handles
    // the fetch request.
    WorkingSetID _idBeingPagedIn;

    CommonStats _commonStats;
    IDHackStats _specificStats;
};

}  // namespace mongo
