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

#include "mongo/platform/basic.h"

#include "mongo/db/query/explain.h"

#include <boost/scoped_ptr.hpp>

#include "mongo/base/owned_pointer_vector.h"
#include "mongo/db/exec/multi_plan.h"
#include "mongo/db/query/get_executor.h"
#include "mongo/db/query/plan_executor.h"
#include "mongo/db/query/query_planner.h"
#include "mongo/db/query/query_settings.h"
#include "mongo/db/query/stage_builder.h"
#include "mongo/db/exec/working_set_common.h"
#include "mongo/db/server_options.h"
#include "mongo/db/server_parameters.h"
#include "mongo/util/mongoutils/str.h"
#include "mongo/util/version.h"

namespace {

using namespace mongo;
using boost::scoped_ptr;
using std::auto_ptr;
using std::string;
using std::vector;

/**
 * Traverse the tree rooted at 'root', and add all tree nodes into the list 'flattened'.
 */
void flattenStatsTree(PlanStageStats* root, vector<PlanStageStats*>* flattened) {
    flattened->push_back(root);
    for (size_t i = 0; i < root->children.size(); ++i) {
        flattenStatsTree(root->children[i], flattened);
    }
}

/**
 * Traverse the tree rooted at 'root', and add all nodes into the list 'flattened'.
 */
void flattenExecTree(PlanStage* root, vector<PlanStage*>* flattened) {
    flattened->push_back(root);
    vector<PlanStage*> children = root->getChildren();
    for (size_t i = 0; i < children.size(); ++i) {
        flattenExecTree(children[i], flattened);
    }
}

/**
 * Get a pointer to the MultiPlanStage inside the stage tree rooted at 'root'.
 * Returns NULL if there is no MPS.
 */
MultiPlanStage* getMultiPlanStage(PlanStage* root) {
    if (root->stageType() == STAGE_MULTI_PLAN) {
        MultiPlanStage* mps = static_cast<MultiPlanStage*>(root);
        return mps;
    }

    vector<PlanStage*> children = root->getChildren();
    for (size_t i = 0; i < children.size(); i++) {
        MultiPlanStage* mps = getMultiPlanStage(children[i]);
        if (mps != NULL) {
            return mps;
        }
    }

    return NULL;
}

/**
 * Given the SpecificStats object for a stage and the type of the stage, returns the
 * number of index keys examined by the stage.
 *
 * This is used for getting the total number of keys examined by a plan. We need
 * to collect a 'totalKeysExamined' metric for a regular explain (in which case this
 * gets called from Explain::generateExecStats()) or for the slow query log / profiler
 * (in which case this gets called from Explain::getSummaryStats()).
 */
size_t getKeysExamined(StageType type, const SpecificStats* specific) {
    if (STAGE_IXSCAN == type) {
        const IndexScanStats* spec = static_cast<const IndexScanStats*>(specific);
        return spec->keysExamined;
    } else if (STAGE_IDHACK == type) {
        const IDHackStats* spec = static_cast<const IDHackStats*>(specific);
        return spec->keysExamined;
    } else if (STAGE_TEXT == type) {
        const TextStats* spec = static_cast<const TextStats*>(specific);
        return spec->keysExamined;
    } else if (STAGE_COUNT_SCAN == type) {
        const CountScanStats* spec = static_cast<const CountScanStats*>(specific);
        return spec->keysExamined;
    } else if (STAGE_DISTINCT == type) {
        const DistinctScanStats* spec = static_cast<const DistinctScanStats*>(specific);
        return spec->keysExamined;
    }

    return 0;
}

/**
 * Given the SpecificStats object for a stage and the type of the stage, returns the
 * number of documents examined by the stage.
 *
 * This is used for getting the total number of documents examined by a plan. We need
 * to collect a 'totalDocsExamined' metric for a regular explain (in which case this
 * gets called from Explain::generateExecStats()) or for the slow query log / profiler
 * (in which case this gets called from Explain::getSummaryStats()).
 */
size_t getDocsExamined(StageType type, const SpecificStats* specific) {
    if (STAGE_IDHACK == type) {
        const IDHackStats* spec = static_cast<const IDHackStats*>(specific);
        return spec->docsExamined;
    } else if (STAGE_TEXT == type) {
        const TextStats* spec = static_cast<const TextStats*>(specific);
        return spec->fetches;
    } else if (STAGE_FETCH == type) {
        const FetchStats* spec = static_cast<const FetchStats*>(specific);
        return spec->docsExamined;
    } else if (STAGE_COLLSCAN == type) {
        const CollectionScanStats* spec = static_cast<const CollectionScanStats*>(specific);
        return spec->docsTested;
    }

    return 0;
}

/**
 * Adds to the plan summary string being built by 'ss' for the execution stage 'stage'.
 */
void addStageSummaryStr(PlanStage* stage, mongoutils::str::stream& ss) {
    // First add the stage type string.
    const CommonStats* common = stage->getCommonStats();
    ss << common->stageTypeStr;

    // Some leaf nodes also provide info about the index they used.
    const SpecificStats* specific = stage->getSpecificStats();
    if (STAGE_COUNT_SCAN == stage->stageType()) {
        const CountScanStats* spec = static_cast<const CountScanStats*>(specific);
        ss << " " << spec->keyPattern;
    } else if (STAGE_DISTINCT == stage->stageType()) {
        const DistinctScanStats* spec = static_cast<const DistinctScanStats*>(specific);
        ss << " " << spec->keyPattern;
    } else if (STAGE_GEO_NEAR_2D == stage->stageType()) {
        const NearStats* spec = static_cast<const NearStats*>(specific);
        ss << " " << spec->keyPattern;
    } else if (STAGE_GEO_NEAR_2DSPHERE == stage->stageType()) {
        const NearStats* spec = static_cast<const NearStats*>(specific);
        ss << " " << spec->keyPattern;
    } else if (STAGE_IXSCAN == stage->stageType()) {
        const IndexScanStats* spec = static_cast<const IndexScanStats*>(specific);
        ss << " " << spec->keyPattern;
    } else if (STAGE_TEXT == stage->stageType()) {
        const TextStats* spec = static_cast<const TextStats*>(specific);
        ss << " " << spec->indexPrefix;
    }
}

}  // namespace

namespace mongo {

using mongoutils::str::stream;

// static
void Explain::statsToBSON(const PlanStageStats& stats,
                          ExplainCommon::Verbosity verbosity,
                          BSONObjBuilder* bob,
                          BSONObjBuilder* topLevelBob) {
    invariant(bob);
    invariant(topLevelBob);

    // Stop as soon as the BSON object we're building exceeds 10 MB.
    static const int kMaxStatsBSONSize = 10 * 1024 * 1024;
    if (topLevelBob->len() > kMaxStatsBSONSize) {
        bob->append("warning", "stats tree exceeded 10 MB");
        return;
    }

    // Stage name.
    bob->append("stage", stats.common.stageTypeStr);

    // Display the BSON representation of the filter, if there is one.
    if (!stats.common.filter.isEmpty()) {
        bob->append("filter", stats.common.filter);
    }

    // Some top-level exec stats get pulled out of the root stage.
    if (verbosity >= ExplainCommon::EXEC_STATS) {
        bob->appendNumber("nReturned", stats.common.advanced);
        bob->appendNumber("executionTimeMillisEstimate", stats.common.executionTimeMillis);
        bob->appendNumber("works", stats.common.works);
        bob->appendNumber("advanced", stats.common.advanced);
        bob->appendNumber("needTime", stats.common.needTime);
        bob->appendNumber("needFetch", stats.common.needFetch);
        bob->appendNumber("saveState", stats.common.yields);
        bob->appendNumber("restoreState", stats.common.unyields);
        bob->appendNumber("isEOF", stats.common.isEOF);
        bob->appendNumber("invalidates", stats.common.invalidates);
    }

    // Stage-specific stats
    if (STAGE_AND_HASH == stats.stageType) {
        AndHashStats* spec = static_cast<AndHashStats*>(stats.specific.get());

        if (verbosity >= ExplainCommon::EXEC_STATS) {
            bob->appendNumber("memUsage", spec->memUsage);
            bob->appendNumber("memLimit", spec->memLimit);

            bob->appendNumber("flaggedButPassed", spec->flaggedButPassed);
            bob->appendNumber("flaggedInProgress", spec->flaggedInProgress);
            for (size_t i = 0; i < spec->mapAfterChild.size(); ++i) {
                bob->appendNumber(string(stream() << "mapAfterChild_" << i),
                                  spec->mapAfterChild[i]);
            }
        }
    } else if (STAGE_AND_SORTED == stats.stageType) {
        AndSortedStats* spec = static_cast<AndSortedStats*>(stats.specific.get());

        if (verbosity >= ExplainCommon::EXEC_STATS) {
            bob->appendNumber("flagged", spec->flagged);
            bob->appendNumber("matchTested", spec->matchTested);
            for (size_t i = 0; i < spec->failedAnd.size(); ++i) {
                bob->appendNumber(string(stream() << "failedAnd_" << i), spec->failedAnd[i]);
            }
        }
    } else if (STAGE_COLLSCAN == stats.stageType) {
        CollectionScanStats* spec = static_cast<CollectionScanStats*>(stats.specific.get());
        bob->append("direction", spec->direction > 0 ? "forward" : "backward");
        if (verbosity >= ExplainCommon::EXEC_STATS) {
            bob->appendNumber("docsExamined", spec->docsTested);
        }
    } else if (STAGE_COUNT == stats.stageType) {
        CountStats* spec = static_cast<CountStats*>(stats.specific.get());

        if (verbosity >= ExplainCommon::EXEC_STATS) {
            bob->appendNumber("nCounted", spec->nCounted);
            bob->appendNumber("nSkipped", spec->nSkipped);
        }
    } else if (STAGE_COUNT_SCAN == stats.stageType) {
        CountScanStats* spec = static_cast<CountScanStats*>(stats.specific.get());

        if (verbosity >= ExplainCommon::EXEC_STATS) {
            bob->appendNumber("keysExamined", spec->keysExamined);
        }

        bob->append("keyPattern", spec->keyPattern);
        bob->append("indexName", spec->indexName);
        bob->appendBool("isMultiKey", spec->isMultiKey);
    } else if (STAGE_DELETE == stats.stageType) {
        DeleteStats* spec = static_cast<DeleteStats*>(stats.specific.get());

        if (verbosity >= ExplainCommon::EXEC_STATS) {
            bob->appendNumber("nWouldDelete", spec->docsDeleted);
            bob->appendNumber("nInvalidateSkips", spec->nInvalidateSkips);
        }
    } else if (STAGE_FETCH == stats.stageType) {
        FetchStats* spec = static_cast<FetchStats*>(stats.specific.get());
        if (verbosity >= ExplainCommon::EXEC_STATS) {
            bob->appendNumber("docsExamined", spec->docsExamined);
            bob->appendNumber("alreadyHasObj", spec->alreadyHasObj);
        }
    } else if (STAGE_GEO_NEAR_2D == stats.stageType || STAGE_GEO_NEAR_2DSPHERE == stats.stageType) {
        NearStats* spec = static_cast<NearStats*>(stats.specific.get());

        bob->append("keyPattern", spec->keyPattern);
        bob->append("indexName", spec->indexName);

        if (verbosity >= ExplainCommon::EXEC_STATS) {
            BSONArrayBuilder intervalsBob(bob->subarrayStart("searchIntervals"));
            for (vector<IntervalStats>::const_iterator it = spec->intervalStats.begin();
                 it != spec->intervalStats.end();
                 ++it) {
                BSONObjBuilder intervalBob(intervalsBob.subobjStart());
                intervalBob.append("minDistance", it->minDistanceAllowed);
                intervalBob.append("maxDistance", it->maxDistanceAllowed);
                intervalBob.append("maxInclusive", it->inclusiveMaxDistanceAllowed);
            }
            intervalsBob.doneFast();
        }
    } else if (STAGE_GROUP == stats.stageType) {
        GroupStats* spec = static_cast<GroupStats*>(stats.specific.get());
        if (verbosity >= ExplainCommon::EXEC_STATS) {
            bob->appendNumber("nGroups", spec->nGroups);
        }
    } else if (STAGE_IDHACK == stats.stageType) {
        IDHackStats* spec = static_cast<IDHackStats*>(stats.specific.get());
        if (verbosity >= ExplainCommon::EXEC_STATS) {
            bob->appendNumber("keysExamined", spec->keysExamined);
            bob->appendNumber("docsExamined", spec->docsExamined);
        }
    } else if (STAGE_IXSCAN == stats.stageType) {
        IndexScanStats* spec = static_cast<IndexScanStats*>(stats.specific.get());

        bob->append("keyPattern", spec->keyPattern);
        bob->append("indexName", spec->indexName);
        bob->appendBool("isMultiKey", spec->isMultiKey);
        bob->append("direction", spec->direction > 0 ? "forward" : "backward");

        if ((topLevelBob->len() + spec->indexBounds.objsize()) > kMaxStatsBSONSize) {
            bob->append("warning", "index bounds omitted due to BSON size limit");
        } else {
            bob->append("indexBounds", spec->indexBounds);
        }

        if (verbosity >= ExplainCommon::EXEC_STATS) {
            bob->appendNumber("keysExamined", spec->keysExamined);
            bob->appendNumber("dupsTested", spec->dupsTested);
            bob->appendNumber("dupsDropped", spec->dupsDropped);
            bob->appendNumber("seenInvalidated", spec->seenInvalidated);
            bob->appendNumber("matchTested", spec->matchTested);
        }
    } else if (STAGE_OR == stats.stageType) {
        OrStats* spec = static_cast<OrStats*>(stats.specific.get());

        if (verbosity >= ExplainCommon::EXEC_STATS) {
            bob->appendNumber("dupsTested", spec->dupsTested);
            bob->appendNumber("dupsDropped", spec->dupsDropped);
            bob->appendNumber("locsForgotten", spec->locsForgotten);
            for (size_t i = 0; i < spec->matchTested.size(); ++i) {
                bob->appendNumber(string(stream() << "matchTested_" << i), spec->matchTested[i]);
            }
        }
    } else if (STAGE_LIMIT == stats.stageType) {
        LimitStats* spec = static_cast<LimitStats*>(stats.specific.get());
        bob->appendNumber("limitAmount", spec->limit);
    } else if (STAGE_PROJECTION == stats.stageType) {
        ProjectionStats* spec = static_cast<ProjectionStats*>(stats.specific.get());
        bob->append("transformBy", spec->projObj);
    } else if (STAGE_SHARDING_FILTER == stats.stageType) {
        ShardingFilterStats* spec = static_cast<ShardingFilterStats*>(stats.specific.get());

        if (verbosity >= ExplainCommon::EXEC_STATS) {
            bob->appendNumber("chunkSkips", spec->chunkSkips);
        }
    } else if (STAGE_SKIP == stats.stageType) {
        SkipStats* spec = static_cast<SkipStats*>(stats.specific.get());
        bob->appendNumber("skipAmount", spec->skip);
    } else if (STAGE_SORT == stats.stageType) {
        SortStats* spec = static_cast<SortStats*>(stats.specific.get());
        bob->append("sortPattern", spec->sortPattern);

        if (verbosity >= ExplainCommon::EXEC_STATS) {
            bob->appendNumber("memUsage", spec->memUsage);
            bob->appendNumber("memLimit", spec->memLimit);
        }

        if (spec->limit > 0) {
            bob->appendNumber("limitAmount", spec->limit);
        }
    } else if (STAGE_SORT_MERGE == stats.stageType) {
        MergeSortStats* spec = static_cast<MergeSortStats*>(stats.specific.get());
        bob->append("sortPattern", spec->sortPattern);

        if (verbosity >= ExplainCommon::EXEC_STATS) {
            bob->appendNumber("dupsTested", spec->dupsTested);
            bob->appendNumber("dupsDropped", spec->dupsDropped);
        }
    } else if (STAGE_TEXT == stats.stageType) {
        TextStats* spec = static_cast<TextStats*>(stats.specific.get());

        if (verbosity >= ExplainCommon::EXEC_STATS) {
            bob->appendNumber("keysExamined", spec->keysExamined);
            bob->appendNumber("docsExamined", spec->fetches);
        }

        bob->append("indexPrefix", spec->indexPrefix);
        bob->append("indexName", spec->indexName);
        bob->append("parsedTextQuery", spec->parsedTextQuery);
    } else if (STAGE_UPDATE == stats.stageType) {
        UpdateStats* spec = static_cast<UpdateStats*>(stats.specific.get());

        if (verbosity >= ExplainCommon::EXEC_STATS) {
            bob->appendNumber("nMatched", spec->nMatched);
            bob->appendNumber("nWouldModify", spec->nModified);
            bob->appendNumber("nInvalidateSkips", spec->nInvalidateSkips);
            bob->appendBool("wouldInsert", spec->inserted);
            bob->appendBool("fastmod", spec->fastmod);
            bob->appendBool("fastmodinsert", spec->fastmodinsert);
        }
    }

    // We're done if there are no children.
    if (stats.children.empty()) {
        return;
    }

    // If there's just one child (a common scenario), avoid making an array. This makes
    // the output more readable by saving a level of nesting. Name the field 'inputStage'
    // rather than 'inputStages'.
    if (1 == stats.children.size()) {
        BSONObjBuilder childBob;
        statsToBSON(*stats.children[0], verbosity, &childBob, topLevelBob);
        bob->append("inputStage", childBob.obj());
        return;
    }

    // There is more than one child. Recursively call statsToBSON(...) on each
    // of them and add them to the 'inputStages' array.

    BSONArrayBuilder childrenBob(bob->subarrayStart("inputStages"));
    for (size_t i = 0; i < stats.children.size(); ++i) {
        BSONObjBuilder childBob(childrenBob.subobjStart());
        statsToBSON(*stats.children[i], verbosity, &childBob, topLevelBob);
    }
    childrenBob.doneFast();
}

// static
BSONObj Explain::statsToBSON(const PlanStageStats& stats, ExplainCommon::Verbosity verbosity) {
    BSONObjBuilder bob;
    statsToBSON(stats, &bob, verbosity);
    return bob.obj();
}

// static
void Explain::statsToBSON(const PlanStageStats& stats,
                          BSONObjBuilder* bob,
                          ExplainCommon::Verbosity verbosity) {
    statsToBSON(stats, verbosity, bob, bob);
}

// static
void Explain::generatePlannerInfo(PlanExecutor* exec,
                                  PlanStageStats* winnerStats,
                                  const vector<PlanStageStats*>& rejectedStats,
                                  BSONObjBuilder* out) {
    CanonicalQuery* query = exec->getCanonicalQuery();

    BSONObjBuilder plannerBob(out->subobjStart("queryPlanner"));
    ;

    plannerBob.append("plannerVersion", QueryPlanner::kPlannerVersion);
    plannerBob.append("namespace", exec->ns());

    // Find whether there is an index filter set for the query shape. The 'indexFilterSet'
    // field will always be false in the case of EOF or idhack plans.
    bool indexFilterSet = false;
    if (exec->collection() && exec->getCanonicalQuery()) {
        const Collection* collection = exec->collection();
        QuerySettings* querySettings = collection->infoCache()->getQuerySettings();
        AllowedIndices* allowedIndicesRaw;
        if (querySettings->getAllowedIndices(*exec->getCanonicalQuery(), &allowedIndicesRaw)) {
            // Found an index filter set on the query shape.
            boost::scoped_ptr<AllowedIndices> allowedIndices(allowedIndicesRaw);
            indexFilterSet = true;
        }
    }
    plannerBob.append("indexFilterSet", indexFilterSet);

    // In general we should have a canonical query, but sometimes we may avoid
    // creating a canonical query as an optimization (specifically, the update system
    // does not canonicalize for idhack updates). In these cases, 'query' is NULL.
    if (NULL != query) {
        BSONObjBuilder parsedQueryBob(plannerBob.subobjStart("parsedQuery"));
        query->root()->toBSON(&parsedQueryBob);
        parsedQueryBob.doneFast();
    }

    BSONObjBuilder winningPlanBob(plannerBob.subobjStart("winningPlan"));
    statsToBSON(*winnerStats, &winningPlanBob, ExplainCommon::QUERY_PLANNER);
    winningPlanBob.doneFast();

    // Genenerate array of rejected plans.
    BSONArrayBuilder allPlansBob(plannerBob.subarrayStart("rejectedPlans"));
    for (size_t i = 0; i < rejectedStats.size(); i++) {
        BSONObjBuilder childBob(allPlansBob.subobjStart());
        statsToBSON(*rejectedStats[i], &childBob, ExplainCommon::QUERY_PLANNER);
    }
    allPlansBob.doneFast();

    plannerBob.doneFast();
}

// static
void Explain::generateExecStats(PlanStageStats* stats,
                                ExplainCommon::Verbosity verbosity,
                                BSONObjBuilder* out,
                                long long totalTimeMillis) {
    out->appendNumber("nReturned", stats->common.advanced);

    // Time elapsed could might be either precise or approximate.
    if (totalTimeMillis >= 0) {
        out->appendNumber("executionTimeMillis", totalTimeMillis);
    } else {
        out->appendNumber("executionTimeMillisEstimate", stats->common.executionTimeMillis);
    }

    // Flatten the stats tree into a list.
    vector<PlanStageStats*> statsNodes;
    flattenStatsTree(stats, &statsNodes);

    // Iterate over all stages in the tree and get the total number of keys/docs examined.
    // These are just aggregations of information already available in the stats tree.
    size_t totalKeysExamined = 0;
    size_t totalDocsExamined = 0;
    for (size_t i = 0; i < statsNodes.size(); ++i) {
        totalKeysExamined +=
            getKeysExamined(statsNodes[i]->stageType, statsNodes[i]->specific.get());
        totalDocsExamined +=
            getDocsExamined(statsNodes[i]->stageType, statsNodes[i]->specific.get());
    }

    out->appendNumber("totalKeysExamined", totalKeysExamined);
    out->appendNumber("totalDocsExamined", totalDocsExamined);

    // Add the tree of stages, with individual execution stats for each stage.
    BSONObjBuilder stagesBob(out->subobjStart("executionStages"));
    statsToBSON(*stats, &stagesBob, verbosity);
    stagesBob.doneFast();
}

// static
void Explain::generateServerInfo(BSONObjBuilder* out) {
    BSONObjBuilder serverBob(out->subobjStart("serverInfo"));
    out->append("host", getHostNameCached());
    out->appendNumber("port", serverGlobalParams.port);
    out->append("version", versionString);
    out->append("gitVersion", gitVersion());
    serverBob.doneFast();
}

// static
void Explain::explainStages(PlanExecutor* exec,
                            ExplainCommon::Verbosity verbosity,
                            BSONObjBuilder* out) {
    //
    // Step 1: run the stages as required by the verbosity level.
    //

    // Inspect the tree to see if there is a MultiPlanStage.
    MultiPlanStage* mps = getMultiPlanStage(exec->getRootStage());

    // Get stats of the winning plan from the trial period, if the verbosity level
    // is high enough and there was a runoff between multiple plans.
    auto_ptr<PlanStageStats> winningStatsTrial;
    if (verbosity >= ExplainCommon::EXEC_ALL_PLANS && NULL != mps) {
        winningStatsTrial.reset(exec->getStats());
        invariant(winningStatsTrial.get());
    }

    // If we need execution stats, then run the plan in order to gather the stats.
    Status executePlanStatus = Status::OK();
    if (verbosity >= ExplainCommon::EXEC_STATS) {
        executePlanStatus = exec->executePlan();
    }

    //
    // Step 2: collect plan stats (which also give the structure of the plan tree).
    //

    // Get stats for the winning plan.
    scoped_ptr<PlanStageStats> winningStats(exec->getStats());

    // Get stats for the rejected plans, if more than one plan was considered.
    OwnedPointerVector<PlanStageStats> allPlansStats;
    if (NULL != mps) {
        allPlansStats = mps->generateCandidateStats();
    }

    //
    // Step 3: use the stats trees to produce explain BSON.
    //

    if (verbosity >= ExplainCommon::QUERY_PLANNER) {
        generatePlannerInfo(exec, winningStats.get(), allPlansStats.vector(), out);
    }

    if (verbosity >= ExplainCommon::EXEC_STATS) {
        BSONObjBuilder execBob(out->subobjStart("executionStats"));

        // If there is an execution error while running the query, the error is reported under
        // the "executionStats" section and the explain as a whole succeeds.
        execBob.append("executionSuccess", executePlanStatus.isOK());
        if (!executePlanStatus.isOK()) {
            execBob.append("errorMessage", executePlanStatus.reason());
            execBob.append("errorCode", executePlanStatus.code());
        }

        // Generate exec stats BSON for the winning plan.
        OperationContext* opCtx = exec->getOpCtx();
        long long totalTimeMillis = opCtx->getCurOp()->elapsedMillis();
        generateExecStats(winningStats.get(), verbosity, &execBob, totalTimeMillis);

        // Also generate exec stats for all plans, if the verbosity level is high enough.
        // These stats reflect what happened during the trial period that ranked the plans.
        if (verbosity >= ExplainCommon::EXEC_ALL_PLANS) {
            // If we ranked multiple plans against each other, then add stats collected
            // from the trial period of the winning plan. The "allPlansExecution" section
            // will contain an apples-to-apples comparison of the winning plan's stats against
            // all rejected plans' stats collected during the trial period.
            if (NULL != mps) {
                invariant(winningStatsTrial.get());
                allPlansStats.push_back(winningStatsTrial.release());
            }

            BSONArrayBuilder allPlansBob(execBob.subarrayStart("allPlansExecution"));
            for (size_t i = 0; i < allPlansStats.size(); ++i) {
                BSONObjBuilder planBob(allPlansBob.subobjStart());
                generateExecStats(allPlansStats[i], verbosity, &planBob);
                planBob.doneFast();
            }
            allPlansBob.doneFast();
        }

        execBob.doneFast();
    }

    generateServerInfo(out);
}

// static
string Explain::getPlanSummary(PlanExecutor* exec) {
    return getPlanSummary(exec->getRootStage());
}

// static
string Explain::getPlanSummary(PlanStage* root) {
    vector<PlanStage*> stages;
    flattenExecTree(root, &stages);

    // Use this stream to build the plan summary string.
    mongoutils::str::stream ss;
    bool seenLeaf = false;

    for (size_t i = 0; i < stages.size(); i++) {
        if (stages[i]->getChildren().empty()) {
            // This is a leaf node. Add to the plan summary string accordingly. Unless
            // this is the first leaf we've seen, add a delimiting string first.
            if (seenLeaf) {
                ss << ", ";
            } else {
                seenLeaf = true;
            }
            addStageSummaryStr(stages[i], ss);
        }
    }

    return ss;
}

// static
void Explain::getSummaryStats(PlanExecutor* exec, PlanSummaryStats* statsOut) {
    invariant(NULL != statsOut);

    PlanStage* root = exec->getRootStage();

    // We can get some of the fields we need from the common stats stored in the
    // root stage of the plan tree.
    const CommonStats* common = root->getCommonStats();
    statsOut->nReturned = common->advanced;
    statsOut->executionTimeMillis = common->executionTimeMillis;

    // The other fields are aggregations over the stages in the plan tree. We flatten
    // the tree into a list and then compute these aggregations.
    vector<PlanStage*> stages;
    flattenExecTree(root, &stages);

    for (size_t i = 0; i < stages.size(); i++) {
        statsOut->totalKeysExamined +=
            getKeysExamined(stages[i]->stageType(), stages[i]->getSpecificStats());
        statsOut->totalDocsExamined +=
            getDocsExamined(stages[i]->stageType(), stages[i]->getSpecificStats());

        if (STAGE_IDHACK == stages[i]->stageType()) {
            statsOut->isIdhack = true;
        }
        if (STAGE_SORT == stages[i]->stageType()) {
            statsOut->hasSortStage = true;
        }
    }
}

}  // namespace mongo
