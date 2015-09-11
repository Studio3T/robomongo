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

#include "mongo/db/query/canonical_query.h"
#include "mongo/db/query/plan_executor.h"
#include "mongo/db/query/query_planner_params.h"
#include "mongo/db/query/query_settings.h"
#include "mongo/db/query/query_solution.h"
#include "mongo/db/ops/delete_request.h"
#include "mongo/db/ops/parsed_delete.h"
#include "mongo/db/ops/parsed_update.h"
#include "mongo/db/ops/update_driver.h"
#include "mongo/db/ops/update_request.h"

namespace mongo {

class Collection;
struct CountRequest;

struct GroupRequest;

/**
 * Filter indexes retrieved from index catalog by
 * allowed indices in query settings.
 * Used by getExecutor().
 * This function is public to facilitate testing.
 */
void filterAllowedIndexEntries(const AllowedIndices& allowedIndices,
                               std::vector<IndexEntry>* indexEntries);

/**
 * Fill out the provided 'plannerParams' for the 'canonicalQuery' operating on the collection
 * 'collection'.  Exposed for testing.
 */
void fillOutPlannerParams(OperationContext* txn,
                          Collection* collection,
                          CanonicalQuery* canonicalQuery,
                          QueryPlannerParams* plannerParams);

/**
 * Get a plan executor for a query. Takes ownership of 'rawCanonicalQuery'.
 *
 * If the query is valid and an executor could be created, returns Status::OK()
 * and populates *out with the PlanExecutor.
 *
 * If the query cannot be executed, returns a Status indicating why.
 */
Status getExecutor(OperationContext* txn,
                   Collection* collection,
                   CanonicalQuery* rawCanonicalQuery,
                   PlanExecutor::YieldPolicy yieldPolicy,
                   PlanExecutor** out,
                   size_t plannerOptions = 0);

/**
 * Get a plan executor for query. This differs from the getExecutor(...) function
 * above in that the above requires a non-NULL canonical query, whereas this
 * function can retrieve a plan executor from the raw query object.
 *
 * Used to support idhack updates that do not create a canonical query.
 *
 * If the query is valid and an executor could be created, returns Status::OK()
 * and populates *out with the PlanExecutor.
 *
 * If the query cannot be executed, returns a Status indicating why.
 */
Status getExecutor(OperationContext* txn,
                   Collection* collection,
                   const std::string& ns,
                   const BSONObj& unparsedQuery,
                   PlanExecutor::YieldPolicy yieldPolicy,
                   PlanExecutor** out,
                   size_t plannerOptions = 0);

/**
 * If possible, turn the provided QuerySolution into a QuerySolution that uses a DistinctNode
 * to provide results for the distinct command.
 *
 * If the provided solution could be mutated successfully, returns true, otherwise returns
 * false.
 */
bool turnIxscanIntoDistinctIxscan(QuerySolution* soln, const std::string& field);

/*
 * Get an executor for a query executing as part of a distinct command.
 *
 * Distinct is unique in that it doesn't care about getting all the results; it just wants all
 * possible values of a certain field.  As such, we can skip lots of data in certain cases (see
 * body of method for detail).
 */
Status getExecutorDistinct(OperationContext* txn,
                           Collection* collection,
                           const BSONObj& query,
                           const std::string& field,
                           PlanExecutor::YieldPolicy yieldPolicy,
                           PlanExecutor** out);

/*
 * Get a PlanExecutor for a query executing as part of a count command.
 *
 * Count doesn't care about actually examining its results; it just wants to walk through them.
 * As such, with certain covered queries, we can skip the overhead of fetching etc. when
 * executing a count.
 */
Status getExecutorCount(OperationContext* txn,
                        Collection* collection,
                        const CountRequest& request,
                        PlanExecutor::YieldPolicy yieldPolicy,
                        PlanExecutor** execOut);

/**
 * Get a PlanExecutor for a delete operation. 'parsedDelete' describes the query predicate
 * and delete flags like 'isMulti'. The caller must hold the appropriate MODE_X or MODE_IX
 * locks, and must not release these locks until after the returned PlanExecutor is deleted.
 *
 * The returned PlanExecutor will yield if and only if parsedDelete->canYield().
 *
 * Does not take ownership of its arguments.
 *
 * If the query is valid and an executor could be created, returns Status::OK() and populates
 * *execOut with the PlanExecutor. The caller takes ownership of *execOut.
 *
 * If the query cannot be executed, returns a Status indicating why.
 */
Status getExecutorDelete(OperationContext* txn,
                         Collection* collection,
                         ParsedDelete* parsedDelete,
                         PlanExecutor** execOut);

/**
 * Get a PlanExecutor for an update operation. 'parsedUpdate' describes the query predicate
 * and update modifiers. The caller must hold the appropriate MODE_X or MODE_IX locks prior
 * to calling this function, and must not release these locks until after the returned
 * PlanExecutor is deleted.
 *
 * The returned PlanExecutor will yield if and only if parsedUpdate->canYield().
 *
 * Does not take ownership of its arguments.
 *
 * If the query is valid and an executor could be created, returns Status::OK() and populates
 * *out with the PlanExecutor. The caller takes ownership of *execOut.
 *
 * If the query cannot be executed, returns a Status indicating why.
 */
Status getExecutorUpdate(OperationContext* txn,
                         Collection* collection,
                         ParsedUpdate* parsedUpdate,
                         OpDebug* opDebug,
                         PlanExecutor** execOut);

/**
 * Get a PlanExecutor for a group operation.  'rawCanonicalQuery' describes the predicate for
 * the documents to be grouped.
 *
 * Takes ownership of 'rawCanonicalQuery'. Does not take ownership of other args.
 *
 * If the query is valid and an executor could be created, returns Status::OK() and populates
 * *out with the PlanExecutor.
 *
 * If an executor could not be created, returns a Status indicating why.
 */
Status getExecutorGroup(OperationContext* txn,
                        Collection* collection,
                        const GroupRequest& request,
                        PlanExecutor::YieldPolicy yieldPolicy,
                        PlanExecutor** execOut);

}  // namespace mongo
