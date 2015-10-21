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

#include "mongo/base/init.h"
#include "mongo/db/auth/action_set.h"
#include "mongo/db/auth/action_type.h"
#include "mongo/db/auth/privilege.h"
#include "mongo/db/catalog/database.h"
#include "mongo/db/commands.h"
#include "mongo/db/client.h"
#include "mongo/db/exec/and_hash.h"
#include "mongo/db/exec/and_sorted.h"
#include "mongo/db/exec/collection_scan.h"
#include "mongo/db/exec/delete.h"
#include "mongo/db/exec/fetch.h"
#include "mongo/db/exec/index_scan.h"
#include "mongo/db/exec/limit.h"
#include "mongo/db/exec/merge_sort.h"
#include "mongo/db/exec/or.h"
#include "mongo/db/exec/skip.h"
#include "mongo/db/exec/sort.h"
#include "mongo/db/exec/text.h"
#include "mongo/db/index/fts_access_method.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/matcher/expression_parser.h"
#include "mongo/db/query/plan_executor.h"

namespace mongo {

using std::auto_ptr;
using std::string;
using std::vector;

/**
 * A command for manually constructing a query tree and running it.
 *
 * db.runCommand({stageDebug: {collection: collname, plan: rootNode}})
 *
 * The value of the filter field is a BSONObj that specifies values that fields must have.  What
 * you'd pass to a matcher.
 *
 * Leaf Nodes:
 *
 * node -> {ixscan: {filter: {FILTER},
 *                   args: {indexKeyPattern: kpObj, start: startObj,
 *                          stop: stopObj, endInclusive: true/false, direction: -1/1,
 *                          limit: int}}}
 * node -> {cscan: {filter: {filter}, args: {direction: -1/1}}}
 * TODO: language for text.
 * node -> {text: {filter: {filter}, args: {search: "searchstr"}}}
 *
 * Internal Nodes:
 *
 * node -> {andHash: {filter: {filter}, args: { nodes: [node, node]}}}
 * node -> {andSorted: {filter: {filter}, args: { nodes: [node, node]}}}
 * node -> {or: {filter: {filter}, args: { dedup:bool, nodes:[node, node]}}}
 * node -> {fetch: {filter: {filter}, args: {node: node}}}
 * node -> {limit: {args: {node: node, num: posint}}}
 * node -> {skip: {args: {node: node, num: posint}}}
 * node -> {sort: {args: {node: node, pattern: objWithSortCriterion }}}
 * node -> {mergeSort: {args: {nodes: [node, node], pattern: objWithSortCriterion}}}
 * node -> {delete: {args: {node: node, isMulti: bool, shouldCallLogOp: bool}}}
 *
 * Forthcoming Nodes:
 *
 * node -> {dedup: {filter: {filter}, args: {node: node, field: field}}}
 * node -> {unwind: {filter: filter}, args: {node: node, field: field}}
 */
class StageDebugCmd : public Command {
public:
    StageDebugCmd() : Command("stageDebug") {}

    virtual bool isWriteCommandForConfigServer() const {
        return false;
    }
    bool slaveOk() const {
        return false;
    }
    bool slaveOverrideOk() const {
        return false;
    }
    void help(std::stringstream& h) const {}

    virtual void addRequiredPrivileges(const std::string& dbname,
                                       const BSONObj& cmdObj,
                                       std::vector<Privilege>* out) {
        // Command is testing-only, and can only be enabled at command line.  Hence, no auth
        // check needed.
    }

    bool run(OperationContext* txn,
             const string& dbname,
             BSONObj& cmdObj,
             int,
             string& errmsg,
             BSONObjBuilder& result,
             bool fromRepl) {
        BSONElement argElt = cmdObj["stageDebug"];
        if (argElt.eoo() || !argElt.isABSONObj()) {
            return false;
        }
        BSONObj argObj = argElt.Obj();

        // Pull out the collection name.
        BSONElement collElt = argObj["collection"];
        if (collElt.eoo() || (String != collElt.type())) {
            return false;
        }
        string collName = collElt.String();

        // Need a context to get the actual Collection*
        // TODO A write lock is currently taken here to accommodate stages that perform writes
        //      (e.g. DeleteStage).  This should be changed to use a read lock for read-only
        //      execution trees.
        ScopedTransaction transaction(txn, MODE_IX);
        Lock::DBLock lk(txn->lockState(), dbname, MODE_X);
        Client::Context ctx(txn, dbname);

        // Make sure the collection is valid.
        Database* db = ctx.db();
        Collection* collection = db->getCollection(db->name() + '.' + collName);
        uassert(17446, "Couldn't find the collection " + collName, NULL != collection);

        // Pull out the plan
        BSONElement planElt = argObj["plan"];
        if (planElt.eoo() || !planElt.isABSONObj()) {
            return false;
        }
        BSONObj planObj = planElt.Obj();

        // Parse the plan into these.
        OwnedPointerVector<MatchExpression> exprs;
        auto_ptr<WorkingSet> ws(new WorkingSet());

        PlanStage* userRoot = parseQuery(txn, collection, planObj, ws.get(), &exprs);
        uassert(16911, "Couldn't parse plan from " + cmdObj.toString(), NULL != userRoot);

        // Add a fetch at the top for the user so we can get obj back for sure.
        // TODO: Do we want to do this for the user?  I think so.
        PlanStage* rootFetch = new FetchStage(txn, ws.get(), userRoot, NULL, collection);

        PlanExecutor* rawExec;
        Status execStatus = PlanExecutor::make(
            txn, ws.release(), rootFetch, collection, PlanExecutor::YIELD_AUTO, &rawExec);
        fassert(28536, execStatus);
        boost::scoped_ptr<PlanExecutor> exec(rawExec);

        BSONArrayBuilder resultBuilder(result.subarrayStart("results"));

        for (BSONObj obj; PlanExecutor::ADVANCED == exec->getNext(&obj, NULL);) {
            resultBuilder.append(obj);
        }

        resultBuilder.done();
        return true;
    }

    PlanStage* parseQuery(OperationContext* txn,
                          Collection* collection,
                          BSONObj obj,
                          WorkingSet* workingSet,
                          OwnedPointerVector<MatchExpression>* exprs) {
        BSONElement firstElt = obj.firstElement();
        if (!firstElt.isABSONObj()) {
            return NULL;
        }
        BSONObj paramObj = firstElt.Obj();

        MatchExpression* matcher = NULL;
        BSONObj nodeArgs;

        // Every node has these two fields.
        const string filterTag = "filter";
        const string argsTag = "args";

        BSONObjIterator it(paramObj);
        while (it.more()) {
            BSONElement e = it.next();
            if (!e.isABSONObj()) {
                return NULL;
            }
            BSONObj argObj = e.Obj();
            if (filterTag == e.fieldName()) {
                StatusWithMatchExpression swme = MatchExpressionParser::parse(
                    argObj, WhereCallbackReal(txn, collection->ns().db()));
                if (!swme.isOK()) {
                    return NULL;
                }
                // exprs is what will wind up deleting this.
                matcher = swme.getValue();
                verify(NULL != matcher);
                exprs->mutableVector().push_back(matcher);
            } else if (argsTag == e.fieldName()) {
                nodeArgs = argObj;
            } else {
                uasserted(16910,
                          "Unknown fieldname " + string(e.fieldName()) + " in query node " +
                              obj.toString());
                return NULL;
            }
        }

        string nodeName = firstElt.fieldName();

        if ("ixscan" == nodeName) {
            // This'll throw if it's not an obj but that's OK.
            BSONObj keyPatternObj = nodeArgs["keyPattern"].Obj();

            IndexDescriptor* desc =
                collection->getIndexCatalog()->findIndexByKeyPattern(txn, keyPatternObj);
            uassert(16890, "Can't find index: " + keyPatternObj.toString(), desc);

            IndexScanParams params;
            params.descriptor = desc;
            params.bounds.isSimpleRange = true;
            params.bounds.startKey = nodeArgs["startKey"].Obj();
            params.bounds.endKey = nodeArgs["endKey"].Obj();
            params.bounds.endKeyInclusive = nodeArgs["endKeyInclusive"].Bool();
            params.direction = nodeArgs["direction"].numberInt();

            return new IndexScan(txn, params, workingSet, matcher);
        } else if ("andHash" == nodeName) {
            uassert(
                16921, "Nodes argument must be provided to AND", nodeArgs["nodes"].isABSONObj());

            auto_ptr<AndHashStage> andStage(new AndHashStage(workingSet, matcher, collection));

            int nodesAdded = 0;
            BSONObjIterator it(nodeArgs["nodes"].Obj());
            while (it.more()) {
                BSONElement e = it.next();
                uassert(16922, "node of AND isn't an obj?: " + e.toString(), e.isABSONObj());

                PlanStage* subNode = parseQuery(txn, collection, e.Obj(), workingSet, exprs);
                uassert(
                    16923, "Can't parse sub-node of AND: " + e.Obj().toString(), NULL != subNode);
                // takes ownership
                andStage->addChild(subNode);
                ++nodesAdded;
            }

            uassert(16927, "AND requires more than one child", nodesAdded >= 2);

            return andStage.release();
        } else if ("andSorted" == nodeName) {
            uassert(
                16924, "Nodes argument must be provided to AND", nodeArgs["nodes"].isABSONObj());

            auto_ptr<AndSortedStage> andStage(new AndSortedStage(workingSet, matcher, collection));

            int nodesAdded = 0;
            BSONObjIterator it(nodeArgs["nodes"].Obj());
            while (it.more()) {
                BSONElement e = it.next();
                uassert(16925, "node of AND isn't an obj?: " + e.toString(), e.isABSONObj());

                PlanStage* subNode = parseQuery(txn, collection, e.Obj(), workingSet, exprs);
                uassert(
                    16926, "Can't parse sub-node of AND: " + e.Obj().toString(), NULL != subNode);
                // takes ownership
                andStage->addChild(subNode);
                ++nodesAdded;
            }

            uassert(16928, "AND requires more than one child", nodesAdded >= 2);

            return andStage.release();
        } else if ("or" == nodeName) {
            uassert(
                16934, "Nodes argument must be provided to AND", nodeArgs["nodes"].isABSONObj());
            uassert(16935, "Dedup argument must be provided to OR", !nodeArgs["dedup"].eoo());
            BSONObjIterator it(nodeArgs["nodes"].Obj());
            auto_ptr<OrStage> orStage(new OrStage(workingSet, nodeArgs["dedup"].Bool(), matcher));
            while (it.more()) {
                BSONElement e = it.next();
                if (!e.isABSONObj()) {
                    return NULL;
                }
                PlanStage* subNode = parseQuery(txn, collection, e.Obj(), workingSet, exprs);
                uassert(
                    16936, "Can't parse sub-node of OR: " + e.Obj().toString(), NULL != subNode);
                // takes ownership
                orStage->addChild(subNode);
            }

            return orStage.release();
        } else if ("fetch" == nodeName) {
            uassert(
                16929, "Node argument must be provided to fetch", nodeArgs["node"].isABSONObj());
            PlanStage* subNode =
                parseQuery(txn, collection, nodeArgs["node"].Obj(), workingSet, exprs);
            return new FetchStage(txn, workingSet, subNode, matcher, collection);
        } else if ("limit" == nodeName) {
            uassert(
                16937, "Limit stage doesn't have a filter (put it on the child)", NULL == matcher);
            uassert(
                16930, "Node argument must be provided to limit", nodeArgs["node"].isABSONObj());
            uassert(16931, "Num argument must be provided to limit", nodeArgs["num"].isNumber());
            PlanStage* subNode =
                parseQuery(txn, collection, nodeArgs["node"].Obj(), workingSet, exprs);
            return new LimitStage(nodeArgs["num"].numberInt(), workingSet, subNode);
        } else if ("skip" == nodeName) {
            uassert(
                16938, "Skip stage doesn't have a filter (put it on the child)", NULL == matcher);
            uassert(16932, "Node argument must be provided to skip", nodeArgs["node"].isABSONObj());
            uassert(16933, "Num argument must be provided to skip", nodeArgs["num"].isNumber());
            PlanStage* subNode =
                parseQuery(txn, collection, nodeArgs["node"].Obj(), workingSet, exprs);
            return new SkipStage(nodeArgs["num"].numberInt(), workingSet, subNode);
        } else if ("cscan" == nodeName) {
            CollectionScanParams params;
            params.collection = collection;

            // What direction?
            uassert(16963,
                    "Direction argument must be specified and be a number",
                    nodeArgs["direction"].isNumber());
            if (1 == nodeArgs["direction"].numberInt()) {
                params.direction = CollectionScanParams::FORWARD;
            } else {
                params.direction = CollectionScanParams::BACKWARD;
            }

            return new CollectionScan(txn, params, workingSet, matcher);
        }
// sort is disabled for now.
#if 0
            else if ("sort" == nodeName) {
                uassert(16969, "Node argument must be provided to sort",
                        nodeArgs["node"].isABSONObj());
                uassert(16970, "Pattern argument must be provided to sort",
                        nodeArgs["pattern"].isABSONObj());
                PlanStage* subNode = parseQuery(txn, db, nodeArgs["node"].Obj(), workingSet, exprs);
                SortStageParams params;
                params.pattern = nodeArgs["pattern"].Obj();
                return new SortStage(params, workingSet, subNode);
            }
#endif
        else if ("mergeSort" == nodeName) {
            uassert(
                16971, "Nodes argument must be provided to sort", nodeArgs["nodes"].isABSONObj());
            uassert(16972,
                    "Pattern argument must be provided to sort",
                    nodeArgs["pattern"].isABSONObj());

            MergeSortStageParams params;
            params.pattern = nodeArgs["pattern"].Obj();
            // Dedup is true by default.

            auto_ptr<MergeSortStage> mergeStage(new MergeSortStage(params, workingSet, collection));

            BSONObjIterator it(nodeArgs["nodes"].Obj());
            while (it.more()) {
                BSONElement e = it.next();
                uassert(16973, "node of mergeSort isn't an obj?: " + e.toString(), e.isABSONObj());

                PlanStage* subNode = parseQuery(txn, collection, e.Obj(), workingSet, exprs);
                uassert(16974,
                        "Can't parse sub-node of mergeSort: " + e.Obj().toString(),
                        NULL != subNode);
                // takes ownership
                mergeStage->addChild(subNode);
            }
            return mergeStage.release();
        } else if ("text" == nodeName) {
            string search = nodeArgs["search"].String();

            vector<IndexDescriptor*> idxMatches;
            collection->getIndexCatalog()->findIndexByType(txn, "text", idxMatches);
            uassert(17194, "Expected exactly one text index", idxMatches.size() == 1);

            IndexDescriptor* index = idxMatches[0];
            FTSAccessMethod* fam =
                dynamic_cast<FTSAccessMethod*>(collection->getIndexCatalog()->getIndex(index));
            TextStageParams params(fam->getSpec());
            params.index = index;

            // TODO: Deal with non-empty filters.  This is a hack to put in covering information
            // that can only be checked for equality.  We ignore this now.
            Status s = fam->getSpec().getIndexPrefix(BSONObj(), &params.indexPrefix);
            if (!s.isOK()) {
                // errmsg = s.toString();
                return NULL;
            }

            params.spec = fam->getSpec();

            if (!params.query.parse(search,
                                    fam->getSpec().defaultLanguage().str().c_str(),
                                    fam->getSpec().getTextIndexVersion()).isOK()) {
                return NULL;
            }

            return new TextStage(txn, params, workingSet, matcher);
        } else if ("delete" == nodeName) {
            uassert(
                18636, "Delete stage doesn't have a filter (put it on the child)", NULL == matcher);
            uassert(
                18637, "node argument must be provided to delete", nodeArgs["node"].isABSONObj());
            uassert(18638,
                    "isMulti argument must be provided to delete",
                    nodeArgs["isMulti"].type() == Bool);
            uassert(18639,
                    "shouldCallLogOp argument must be provided to delete",
                    nodeArgs["shouldCallLogOp"].type() == Bool);
            PlanStage* subNode =
                parseQuery(txn, collection, nodeArgs["node"].Obj(), workingSet, exprs);
            DeleteStageParams params;
            params.isMulti = nodeArgs["isMulti"].Bool();
            params.shouldCallLogOp = nodeArgs["shouldCallLogOp"].Bool();
            return new DeleteStage(txn, params, workingSet, collection, subNode);
        } else {
            return NULL;
        }
    }
};

MONGO_INITIALIZER(RegisterStageDebugCmd)(InitializerContext* context) {
    if (Command::testCommandsEnabled) {
        // Leaked intentionally: a Command registers itself when constructed.
        new StageDebugCmd();
    }
    return Status::OK();
}

}  // namespace mongo
