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

#include <boost/scoped_ptr.hpp>
#include <string>

#include "mongo/base/owned_pointer_vector.h"
#include "mongo/base/status.h"
#include "mongo/db/exec/plan_stage.h"
#include "mongo/db/query/canonical_query.h"
#include "mongo/db/query/plan_cache.h"
#include "mongo/db/query/plan_yield_policy.h"
#include "mongo/db/query/query_planner_params.h"
#include "mongo/db/query/query_solution.h"
#include "mongo/db/record_id.h"

namespace mongo {

class OperationContext;

/**
 * The SubplanStage is used for rooted $or queries. It plans each clause of the $or
 * individually, and then creates an overall query plan based on the winning plan from
 * each clause.
 *
 * Uses the MultiPlanStage in order to rank plans for the individual clauses.
 *
 * Notes on caching strategy:
 *
 *   --Interaction with the plan cache is done on a per-clause basis. For a given clause C,
 *   if there is a plan in the cache for shape C, then C is planned using the index tags
 *   obtained from the plan cache entry. If no cached plan is found for C, then a MultiPlanStage
 *   is used to determine the best plan for the clause; unless there is a tie between multiple
 *   candidate plans, the winner is inserted into the plan cache and used to plan subsequent
 *   executions of C. These subsequent executions of shape C could be either as a clause in
 *   another rooted $or query, or shape C as its own query.
 *
 *   --Plans for entire rooted $or queries are neither written to nor read from the plan cache.
 */
class SubplanStage : public PlanStage {
public:
    SubplanStage(OperationContext* txn,
                 Collection* collection,
                 WorkingSet* ws,
                 const QueryPlannerParams& params,
                 CanonicalQuery* cq);

    static bool canUseSubplanning(const CanonicalQuery& query);

    virtual bool isEOF();
    virtual StageState work(WorkingSetID* out);

    virtual void saveState();
    virtual void restoreState(OperationContext* opCtx);
    virtual void invalidate(OperationContext* txn, const RecordId& dl, InvalidationType type);

    virtual std::vector<PlanStage*> getChildren() const;

    virtual StageType stageType() const {
        return STAGE_SUBPLAN;
    }

    PlanStageStats* getStats();

    virtual const CommonStats* getCommonStats();

    virtual const SpecificStats* getSpecificStats();

    static const char* kStageType;

    /**
     * Selects a plan using subplanning. First uses the query planning results from
     * planSubqueries() and the multi plan stage to select the best plan for each branch.
     *
     * If this effort fails, then falls back on planning the whole query normally rather
     * then planning $or branches independently.
     *
     * If 'yieldPolicy' is non-NULL, then all locks may be yielded in between round-robin
     * works of the candidate plans. By default, 'yieldPolicy' is NULL and no yielding will
     * take place.
     *
     * Returns a non-OK status if the plan was killed during yield or if planning fails.
     */
    Status pickBestPlan(PlanYieldPolicy* yieldPolicy);

    //
    // For testing.
    //

    /**
     * Returns true if the i-th branch was planned by retrieving a cached solution,
     * otherwise returns false.
     */
    bool branchPlannedFromCache(size_t i) const;

private:
    /**
     * A class used internally in order to keep track of the results of planning
     * a particular $or branch.
     */
    struct BranchPlanningResult {
        MONGO_DISALLOW_COPYING(BranchPlanningResult);

    public:
        BranchPlanningResult() {}

        // A parsed version of one branch of the $or.
        boost::scoped_ptr<CanonicalQuery> canonicalQuery;

        // If there is cache data available, then we store it here rather than generating
        // a set of alternate plans for the branch. The index tags from the cache data
        // can be applied directly to the parent $or MatchExpression when generating the
        // composite solution.
        boost::scoped_ptr<CachedSolution> cachedSolution;

        // Query solutions resulting from planning the $or branch.
        OwnedPointerVector<QuerySolution> solutions;
    };

    /**
     * Plan each branch of the $or independently, and store the resulting
     * lists of query solutions in '_solutions'.
     *
     * Called from SubplanStage::make so that construction of the subplan stage
     * fails immediately, rather than returning a plan executor and subsequently
     * through getNext(...).
     */
    Status planSubqueries();

    /**
     * Uses the query planning results from planSubqueries() and the multi plan stage
     * to select the best plan for each branch.
     *
     * Helper for pickBestPlan().
     */
    Status choosePlanForSubqueries(PlanYieldPolicy* yieldPolicy);

    /**
     * Used as a fallback if subplanning fails. Helper for pickBestPlan().
     */
    Status choosePlanWholeQuery(PlanYieldPolicy* yieldPolicy);

    // transactional context for read locks. Not owned by us
    OperationContext* _txn;

    Collection* _collection;

    // Not owned here.
    WorkingSet* _ws;

    QueryPlannerParams _plannerParams;

    // Not owned here.
    CanonicalQuery* _query;

    // If we successfully create a "composite solution" by planning each $or branch
    // independently, that solution is owned here.
    boost::scoped_ptr<QuerySolution> _compositeSolution;

    boost::scoped_ptr<PlanStage> _child;

    // Holds a list of the results from planning each branch.
    OwnedPointerVector<BranchPlanningResult> _branchResults;

    // We need this to extract cache-friendly index data from the index assignments.
    std::map<BSONObj, size_t> _indexMap;

    CommonStats _commonStats;
};

}  // namespace mongo
