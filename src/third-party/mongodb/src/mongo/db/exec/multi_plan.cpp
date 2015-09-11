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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kQuery

#include "mongo/platform/basic.h"

#include "mongo/db/exec/multi_plan.h"

#include <algorithm>
#include <math.h>

#include "mongo/base/owned_pointer_vector.h"
#include "mongo/db/catalog/collection.h"
#include "mongo/db/catalog/database.h"
#include "mongo/db/client.h"
#include "mongo/db/exec/scoped_timer.h"
#include "mongo/db/exec/working_set_common.h"
#include "mongo/db/query/explain.h"
#include "mongo/db/query/plan_cache.h"
#include "mongo/db/query/plan_ranker.h"
#include "mongo/db/query/qlog.h"
#include "mongo/db/storage/record_fetcher.h"
#include "mongo/util/mongoutils/str.h"
#include "mongo/util/log.h"

namespace mongo {

using std::auto_ptr;
using std::endl;
using std::list;
using std::vector;

// static
const char* MultiPlanStage::kStageType = "MULTI_PLAN";

MultiPlanStage::MultiPlanStage(OperationContext* txn,
                               const Collection* collection,
                               CanonicalQuery* cq,
                               bool shouldCache)
    : _txn(txn),
      _collection(collection),
      _shouldCache(shouldCache),
      _query(cq),
      _bestPlanIdx(kNoSuchPlan),
      _backupPlanIdx(kNoSuchPlan),
      _failure(false),
      _failureCount(0),
      _statusMemberId(WorkingSet::INVALID_ID),
      _commonStats(kStageType) {}

MultiPlanStage::~MultiPlanStage() {
    for (size_t ix = 0; ix < _candidates.size(); ++ix) {
        delete _candidates[ix].solution;
        delete _candidates[ix].root;
    }
}

void MultiPlanStage::addPlan(QuerySolution* solution, PlanStage* root, WorkingSet* ws) {
    _candidates.push_back(CandidatePlan(solution, root, ws));
}

bool MultiPlanStage::isEOF() {
    if (_failure) {
        return true;
    }

    // If _bestPlanIdx hasn't been found, can't be at EOF
    if (!bestPlanChosen()) {
        return false;
    }

    // We must have returned all our cached results
    // and there must be no more results from the best plan.
    CandidatePlan& bestPlan = _candidates[_bestPlanIdx];
    return bestPlan.results.empty() && bestPlan.root->isEOF();
}

PlanStage::StageState MultiPlanStage::work(WorkingSetID* out) {
    // Adds the amount of time taken by work() to executionTimeMillis.
    ScopedTimer timer(&_commonStats.executionTimeMillis);

    if (_failure) {
        *out = _statusMemberId;
        return PlanStage::FAILURE;
    }

    CandidatePlan& bestPlan = _candidates[_bestPlanIdx];

    // Look for an already produced result that provides the data the caller wants.
    if (!bestPlan.results.empty()) {
        *out = bestPlan.results.front();
        bestPlan.results.pop_front();
        _commonStats.advanced++;
        return PlanStage::ADVANCED;
    }

    // best plan had no (or has no more) cached results

    StageState state = bestPlan.root->work(out);

    if (PlanStage::FAILURE == state && hasBackupPlan()) {
        QLOG() << "Best plan errored out switching to backup\n";
        // Uncache the bad solution if we fall back
        // on the backup solution.
        //
        // XXX: Instead of uncaching we should find a way for the
        // cached plan runner to fall back on a different solution
        // if the best solution fails. Alternatively we could try to
        // defer cache insertion to be after the first produced result.

        _collection->infoCache()->getPlanCache()->remove(*_query);

        _bestPlanIdx = _backupPlanIdx;
        _backupPlanIdx = kNoSuchPlan;

        return _candidates[_bestPlanIdx].root->work(out);
    }

    if (hasBackupPlan() && PlanStage::ADVANCED == state) {
        QLOG() << "Best plan had a blocking stage, became unblocked\n";
        _backupPlanIdx = kNoSuchPlan;
    }

    // Increment stats.
    if (PlanStage::ADVANCED == state) {
        _commonStats.advanced++;
    } else if (PlanStage::NEED_TIME == state) {
        _commonStats.needTime++;
    } else if (PlanStage::NEED_FETCH == state) {
        _commonStats.needFetch++;
    }

    return state;
}

Status MultiPlanStage::tryYield(PlanYieldPolicy* yieldPolicy) {
    // There are two conditions which cause us to yield during plan selection if we have a
    // YIELD_AUTO policy:
    //   1) The yield policy's timer elapsed, or
    //   2) some stage requested a yield due to a document fetch (NEED_FETCH).
    // In both cases, the actual yielding happens here.
    if (NULL != yieldPolicy && (yieldPolicy->shouldYield() || NULL != _fetcher.get())) {
        // Here's where we yield.
        bool alive = yieldPolicy->yield(_fetcher.get());

        if (!alive) {
            _failure = true;
            Status failStat(ErrorCodes::OperationFailed,
                            "PlanExecutor killed during plan selection");
            _statusMemberId = WorkingSetCommon::allocateStatusMember(_candidates[0].ws, failStat);
            return failStat;
        }
    }

    // We're done using the fetcher, so it should be freed. We don't want to
    // use the same RecordFetcher twice.
    _fetcher.reset();

    return Status::OK();
}

// static
size_t MultiPlanStage::getTrialPeriodNumToReturn(const CanonicalQuery& query) {
    // We treat ntoreturn as though it is a limit during plan ranking.
    // This means that ranking might not be great for sort + batchSize.
    // But it also means that we don't buffer too much data for sort + limit.
    // See SERVER-14174 for details.
    size_t numToReturn = query.getParsed().getNumToReturn();

    // Determine the number of results which we will produce during the plan
    // ranking phase before stopping.
    size_t numResults = static_cast<size_t>(internalQueryPlanEvaluationMaxResults);
    if (numToReturn > 0) {
        numResults = std::min(numToReturn, numResults);
    }

    return numResults;
}

Status MultiPlanStage::pickBestPlan(PlanYieldPolicy* yieldPolicy) {
    // Adds the amount of time taken by pickBestPlan() to executionTimeMillis. There's lots of
    // execution work that happens here, so this is needed for the time accounting to
    // make sense.
    ScopedTimer timer(&_commonStats.executionTimeMillis);

    // Run each plan some number of times. This number is at least as great as
    // 'internalQueryPlanEvaluationWorks', but may be larger for big collections.
    size_t numWorks = internalQueryPlanEvaluationWorks;
    if (NULL != _collection) {
        // For large collections, the number of works is set to be this
        // fraction of the collection size.
        double fraction = internalQueryPlanEvaluationCollFraction;

        numWorks = std::max(size_t(internalQueryPlanEvaluationWorks),
                            size_t(fraction * _collection->numRecords(_txn)));
    }

    // We treat ntoreturn as though it is a limit during plan ranking.
    // This means that ranking might not be great for sort + batchSize.
    // But it also means that we don't buffer too much data for sort + limit.
    // See SERVER-14174 for details.
    size_t numResults = getTrialPeriodNumToReturn(*_query);

    // Work the plans, stopping when a plan hits EOF or returns some
    // fixed number of results.
    for (size_t ix = 0; ix < numWorks; ++ix) {
        bool moreToDo = workAllPlans(numResults, yieldPolicy);
        if (!moreToDo) {
            break;
        }
    }

    if (_failure) {
        invariant(WorkingSet::INVALID_ID != _statusMemberId);
        WorkingSetMember* member = _candidates[0].ws->get(_statusMemberId);
        return WorkingSetCommon::getMemberStatus(*member);
    }

    // After picking best plan, ranking will own plan stats from
    // candidate solutions (winner and losers).
    std::auto_ptr<PlanRankingDecision> ranking(new PlanRankingDecision);
    _bestPlanIdx = PlanRanker::pickBestPlan(_candidates, ranking.get());
    verify(_bestPlanIdx >= 0 && _bestPlanIdx < static_cast<int>(_candidates.size()));

    // Copy candidate order. We will need this to sort candidate stats for explain
    // after transferring ownership of 'ranking' to plan cache.
    std::vector<size_t> candidateOrder = ranking->candidateOrder;

    CandidatePlan& bestCandidate = _candidates[_bestPlanIdx];
    std::list<WorkingSetID>& alreadyProduced = bestCandidate.results;
    QuerySolution* bestSolution = bestCandidate.solution;

    QLOG() << "Winning solution:\n" << bestSolution->toString() << endl;
    LOG(2) << "Winning plan: " << Explain::getPlanSummary(bestCandidate.root);

    _backupPlanIdx = kNoSuchPlan;
    if (bestSolution->hasBlockingStage && (0 == alreadyProduced.size())) {
        QLOG() << "Winner has blocking stage, looking for backup plan...\n";
        for (size_t ix = 0; ix < _candidates.size(); ++ix) {
            if (!_candidates[ix].solution->hasBlockingStage) {
                QLOG() << "Candidate " << ix << " is backup child\n";
                _backupPlanIdx = ix;
                break;
            }
        }
    }

    // Logging for tied plans.
    if (ranking->tieForBest && NULL != _collection) {
        // These arrays having two or more entries is implied by 'tieForBest'.
        invariant(ranking->scores.size() > 1);
        invariant(ranking->candidateOrder.size() > 1);

        size_t winnerIdx = ranking->candidateOrder[0];
        size_t runnerUpIdx = ranking->candidateOrder[1];

        LOG(1) << "Winning plan tied with runner-up."
               << " ns: " << _collection->ns() << " " << _query->toStringShort()
               << " winner score: " << ranking->scores[0]
               << " winner summary: " << Explain::getPlanSummary(_candidates[winnerIdx].root)
               << " runner-up score: " << ranking->scores[1]
               << " runner-up summary: " << Explain::getPlanSummary(_candidates[runnerUpIdx].root);

        // There could be more than a 2-way tie, so log the stats for the remaining plans
        // involved in the tie.
        static const double epsilon = 1e-10;
        for (size_t i = 2; i < ranking->scores.size(); i++) {
            if (fabs(ranking->scores[i] - ranking->scores[0]) >= epsilon) {
                break;
            }

            size_t planIdx = ranking->candidateOrder[i];

            LOG(1) << "Plan " << i << " involved in multi-way tie."
                   << " ns: " << _collection->ns() << " " << _query->toStringShort()
                   << " score: " << ranking->scores[i]
                   << " summary: " << Explain::getPlanSummary(_candidates[planIdx].root);
        }
    }

    // If the winning plan produced no results during the ranking period (and, therefore, no
    // plan produced results during the ranking period), then we will not create a plan cache
    // entry.
    if (alreadyProduced.empty() && NULL != _collection) {
        size_t winnerIdx = ranking->candidateOrder[0];
        LOG(1) << "Winning plan had zero results. Not caching."
               << " ns: " << _collection->ns() << " " << _query->toStringShort()
               << " winner score: " << ranking->scores[0]
               << " winner summary: " << Explain::getPlanSummary(_candidates[winnerIdx].root);
    }

    // Store the choice we just made in the cache. In order to do so,
    //   1) the query must be of a type that is safe to cache,
    //   2) two or more plans cannot have tied for the win. Caching in the case of ties can
    //   cause successive queries of the same shape to use a bad index.
    //   3) The caller must have indicated that it is willing to allow a plan to be cached via
    //   the '_shouldCache' argument to the constructor.
    //   4) Furthermore, the winning plan must have returned at least one result. Plans which
    //   return zero results cannot be reliably ranked. Such query shapes are generally
    //   existence type queries, and a winning plan should get cached once the query finds a
    //   result.
    if (PlanCache::shouldCacheQuery(*_query) && !ranking->tieForBest && !alreadyProduced.empty() &&
        _shouldCache) {
        // Create list of candidate solutions for the cache with
        // the best solution at the front.
        std::vector<QuerySolution*> solutions;

        // Generate solutions and ranking decisions sorted by score.
        for (size_t orderingIndex = 0; orderingIndex < candidateOrder.size(); ++orderingIndex) {
            // index into candidates/ranking
            size_t ix = candidateOrder[orderingIndex];
            solutions.push_back(_candidates[ix].solution);
        }

        // Check solution cache data. Do not add to cache if
        // we have any invalid SolutionCacheData data.
        // XXX: One known example is 2D queries
        bool validSolutions = true;
        for (size_t ix = 0; ix < solutions.size(); ++ix) {
            if (NULL == solutions[ix]->cacheData.get()) {
                QLOG() << "Not caching query because this solution has no cache data: "
                       << solutions[ix]->toString();
                validSolutions = false;
                break;
            }
        }

        if (validSolutions) {
            _collection->infoCache()->getPlanCache()->add(*_query, solutions, ranking.release());
        }
    }

    return Status::OK();
}

vector<PlanStageStats*> MultiPlanStage::generateCandidateStats() {
    OwnedPointerVector<PlanStageStats> candidateStats;

    for (size_t ix = 0; ix < _candidates.size(); ix++) {
        if (ix == (size_t)_bestPlanIdx) {
            continue;
        }
        if (ix == (size_t)_backupPlanIdx) {
            continue;
        }

        PlanStageStats* stats = _candidates[ix].root->getStats();
        if (stats) {
            candidateStats.push_back(stats);
        }
    }

    return candidateStats.release();
}

bool MultiPlanStage::workAllPlans(size_t numResults, PlanYieldPolicy* yieldPolicy) {
    bool doneWorking = false;

    for (size_t ix = 0; ix < _candidates.size(); ++ix) {
        CandidatePlan& candidate = _candidates[ix];
        if (candidate.failed) {
            continue;
        }

        // Might need to yield between calls to work due to the timer elapsing.
        if (!(tryYield(yieldPolicy)).isOK()) {
            return false;
        }

        WorkingSetID id = WorkingSet::INVALID_ID;
        PlanStage::StageState state = candidate.root->work(&id);

        if (PlanStage::ADVANCED == state) {
            // Save result for later.
            candidate.results.push_back(id);

            // Once a plan returns enough results, stop working.
            if (candidate.results.size() >= numResults) {
                doneWorking = true;
            }
        } else if (PlanStage::IS_EOF == state) {
            // First plan to hit EOF wins automatically.  Stop evaluating other plans.
            // Assumes that the ranking will pick this plan.
            doneWorking = true;
        } else if (PlanStage::NEED_FETCH == state) {
            // Yielding for a NEED_FETCH is handled above. Here we just make sure that the
            // WSM is fetchable as a sanity check.
            WorkingSetMember* member = candidate.ws->get(id);
            invariant(member->hasFetcher());
            // Transfer ownership of the fetcher and yield.
            _fetcher.reset(member->releaseFetcher());
            if (!(tryYield(yieldPolicy)).isOK()) {
                return false;
            }
        } else if (PlanStage::NEED_TIME != state) {
            // FAILURE or DEAD.  Do we want to just tank that plan and try the rest?  We
            // probably want to fail globally as this shouldn't happen anyway.

            candidate.failed = true;
            ++_failureCount;

            // Propagate most recent seen failure to parent.
            if (PlanStage::FAILURE == state) {
                _statusMemberId = id;
            }

            if (_failureCount == _candidates.size()) {
                _failure = true;
                return false;
            }
        }
    }

    return !doneWorking;
}

void MultiPlanStage::saveState() {
    _txn = NULL;
    for (size_t i = 0; i < _candidates.size(); ++i) {
        _candidates[i].root->saveState();
    }
}

void MultiPlanStage::restoreState(OperationContext* opCtx) {
    invariant(_txn == NULL);
    _txn = opCtx;

    for (size_t i = 0; i < _candidates.size(); ++i) {
        _candidates[i].root->restoreState(opCtx);
    }
}

namespace {

void invalidateHelper(OperationContext* txn,
                      WorkingSet* ws,  // may flag for review
                      const RecordId& dl,
                      list<WorkingSetID>* idsToInvalidate,
                      const Collection* collection) {
    for (list<WorkingSetID>::iterator it = idsToInvalidate->begin();
         it != idsToInvalidate->end();) {
        WorkingSetMember* member = ws->get(*it);
        if (member->hasLoc() && member->loc == dl) {
            list<WorkingSetID>::iterator next = it;
            next++;
            WorkingSetCommon::fetchAndInvalidateLoc(txn, member, collection);
            ws->flagForReview(*it);
            idsToInvalidate->erase(it);
            it = next;
        } else {
            it++;
        }
    }
}
}

void MultiPlanStage::invalidate(OperationContext* txn, const RecordId& dl, InvalidationType type) {
    if (_failure) {
        return;
    }

    if (bestPlanChosen()) {
        CandidatePlan& bestPlan = _candidates[_bestPlanIdx];
        bestPlan.root->invalidate(txn, dl, type);
        invalidateHelper(txn, bestPlan.ws, dl, &bestPlan.results, _collection);
        if (hasBackupPlan()) {
            CandidatePlan& backupPlan = _candidates[_backupPlanIdx];
            backupPlan.root->invalidate(txn, dl, type);
            invalidateHelper(txn, backupPlan.ws, dl, &backupPlan.results, _collection);
        }
    } else {
        for (size_t ix = 0; ix < _candidates.size(); ++ix) {
            _candidates[ix].root->invalidate(txn, dl, type);
            invalidateHelper(txn, _candidates[ix].ws, dl, &_candidates[ix].results, _collection);
        }
    }
}

bool MultiPlanStage::hasBackupPlan() const {
    return kNoSuchPlan != _backupPlanIdx;
}

bool MultiPlanStage::bestPlanChosen() const {
    return kNoSuchPlan != _bestPlanIdx;
}

int MultiPlanStage::bestPlanIdx() const {
    return _bestPlanIdx;
}

QuerySolution* MultiPlanStage::bestSolution() {
    if (_bestPlanIdx == kNoSuchPlan)
        return NULL;

    return _candidates[_bestPlanIdx].solution;
}

vector<PlanStage*> MultiPlanStage::getChildren() const {
    vector<PlanStage*> children;

    if (bestPlanChosen()) {
        children.push_back(_candidates[_bestPlanIdx].root);
    } else {
        for (size_t i = 0; i < _candidates.size(); i++) {
            children.push_back(_candidates[i].root);
        }
    }

    return children;
}

PlanStageStats* MultiPlanStage::getStats() {
    if (bestPlanChosen()) {
        return _candidates[_bestPlanIdx].root->getStats();
    }
    if (hasBackupPlan()) {
        return _candidates[_backupPlanIdx].root->getStats();
    }
    _commonStats.isEOF = isEOF();

    auto_ptr<PlanStageStats> ret(new PlanStageStats(_commonStats, STAGE_MULTI_PLAN));

    return ret.release();
}

const CommonStats* MultiPlanStage::getCommonStats() {
    return &_commonStats;
}

const SpecificStats* MultiPlanStage::getSpecificStats() {
    return &_specificStats;
}

}  // namespace mongo
