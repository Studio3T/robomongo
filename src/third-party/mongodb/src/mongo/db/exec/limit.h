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

#pragma once

#include <boost/scoped_ptr.hpp>

#include "mongo/db/jsobj.h"
#include "mongo/db/exec/plan_stage.h"
#include "mongo/db/record_id.h"

namespace mongo {

/**
 * This stage implements limit functionality.  It only returns 'limit' results before EOF.
 *
 * Sort has a baked-in limit, as it can optimize the sort if it has a limit.
 *
 * Preconditions: None.
 */
class LimitStage : public PlanStage {
public:
    LimitStage(int limit, WorkingSet* ws, PlanStage* child);
    virtual ~LimitStage();

    virtual bool isEOF();
    virtual StageState work(WorkingSetID* out);

    virtual void saveState();
    virtual void restoreState(OperationContext* opCtx);
    virtual void invalidate(OperationContext* txn, const RecordId& dl, InvalidationType type);

    virtual std::vector<PlanStage*> getChildren() const;

    virtual StageType stageType() const {
        return STAGE_LIMIT;
    }

    virtual PlanStageStats* getStats();

    virtual const CommonStats* getCommonStats();

    virtual const SpecificStats* getSpecificStats();

    static const char* kStageType;

private:
    WorkingSet* _ws;
    boost::scoped_ptr<PlanStage> _child;

    // We only return this many results.
    int _numToReturn;

    // Stats
    CommonStats _commonStats;
    LimitStats _specificStats;
};

}  // namespace mongo
