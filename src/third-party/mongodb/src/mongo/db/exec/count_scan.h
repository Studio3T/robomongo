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

#pragma once

#include <boost/scoped_ptr.hpp>

#include "mongo/db/exec/plan_stage.h"
#include "mongo/db/index/btree_index_cursor.h"
#include "mongo/db/index/index_access_method.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/matcher/expression.h"
#include "mongo/db/operation_context.h"
#include "mongo/db/record_id.h"
#include "mongo/platform/unordered_set.h"

namespace mongo {

class IndexAccessMethod;
class IndexDescriptor;
class WorkingSet;

struct CountScanParams {
    CountScanParams() : descriptor(NULL) {}

    // What index are we traversing?
    const IndexDescriptor* descriptor;

    BSONObj startKey;
    bool startKeyInclusive;

    BSONObj endKey;
    bool endKeyInclusive;
};

/**
 * Used by the count command.  Scans an index from a start key to an end key.  Does not create
 * any WorkingSetMember(s) for any of the data, instead returning ADVANCED to indicate to the
 * caller that another result should be counted.
 *
 * Only created through the getExecutorCount path, as count is the only operation that doesn't
 * care about its data.
 */
class CountScan : public PlanStage {
public:
    CountScan(OperationContext* txn, const CountScanParams& params, WorkingSet* workingSet);
    virtual ~CountScan() {}

    virtual StageState work(WorkingSetID* out);
    virtual bool isEOF();
    virtual void saveState();
    virtual void restoreState(OperationContext* opCtx);
    virtual void invalidate(OperationContext* txn, const RecordId& dl, InvalidationType type);

    virtual std::vector<PlanStage*> getChildren() const;

    virtual StageType stageType() const {
        return STAGE_COUNT_SCAN;
    }

    virtual PlanStageStats* getStats();

    virtual const CommonStats* getCommonStats();

    virtual const SpecificStats* getSpecificStats();

    static const char* kStageType;

private:
    /**
     * Initialize the underlying IndexCursor
     */
    void initIndexCursor();

    /**
     * See if we've hit the end yet.
     */
    void checkEnd();

    // transactional context for read locks. Not owned by us
    OperationContext* _txn;

    // The WorkingSet we annotate with results.  Not owned by us.
    WorkingSet* _workingSet;

    // Index access.  Both pointers below are owned by Collection -> IndexCatalog.
    const IndexDescriptor* _descriptor;
    const IndexAccessMethod* _iam;

    // Our start cursor is _btreeCursor.
    boost::scoped_ptr<BtreeIndexCursor> _btreeCursor;

    // Our end marker.
    boost::scoped_ptr<BtreeIndexCursor> _endCursor;

    // Could our index have duplicates?  If so, we use _returned to dedup.
    unordered_set<RecordId, RecordId::Hasher> _returned;

    CountScanParams _params;

    bool _hitEnd;

    bool _shouldDedup;

    CommonStats _commonStats;
    CountScanStats _specificStats;
};

}  // namespace mongo
