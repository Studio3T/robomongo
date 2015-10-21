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
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

/**
 * This file tests PlanExecutor forced yielding, ClientCursor::registerExecutor, and
 * ClientCursor::deregisterExecutor.
 */

#include "mongo/client/dbclientcursor.h"
#include "mongo/db/catalog/collection.h"
#include "mongo/db/catalog/database.h"
#include "mongo/db/dbdirectclient.h"
#include "mongo/db/exec/collection_scan.h"
#include "mongo/db/exec/plan_stage.h"
#include "mongo/db/global_environment_experiment.h"
#include "mongo/db/json.h"
#include "mongo/db/matcher/expression_parser.h"
#include "mongo/db/operation_context_impl.h"
#include "mongo/db/query/plan_executor.h"
#include "mongo/dbtests/dbtests.h"


namespace ExecutorRegistry {

using std::auto_ptr;

class ExecutorRegistryBase {
public:
    ExecutorRegistryBase() : _client(&_opCtx) {
        _ctx.reset(new Client::WriteContext(&_opCtx, ns()));
        _client.dropCollection(ns());

        for (int i = 0; i < N(); ++i) {
            _client.insert(ns(), BSON("foo" << i));
        }
    }

    /**
     * Return a plan executor that is going over the collection in ns().
     */
    PlanExecutor* getCollscan() {
        auto_ptr<WorkingSet> ws(new WorkingSet());
        CollectionScanParams params;
        params.collection = collection();
        params.direction = CollectionScanParams::FORWARD;
        params.tailable = false;
        auto_ptr<CollectionScan> scan(new CollectionScan(&_opCtx, params, ws.get(), NULL));

        // Create a plan executor to hold it
        CanonicalQuery* cq;
        ASSERT(CanonicalQuery::canonicalize(ns(), BSONObj(), &cq).isOK());
        PlanExecutor* exec;
        // Takes ownership of 'ws', 'scan', and 'cq'.
        Status status = PlanExecutor::make(&_opCtx,
                                           ws.release(),
                                           scan.release(),
                                           cq,
                                           _ctx->ctx().db()->getCollection(ns()),
                                           PlanExecutor::YIELD_MANUAL,
                                           &exec);
        ASSERT_OK(status);
        return exec;
    }

    void registerExecutor(PlanExecutor* exec) {
        WriteUnitOfWork wuow(&_opCtx);
        _ctx->ctx()
            .db()
            ->getOrCreateCollection(&_opCtx, ns())
            ->getCursorManager()
            ->registerExecutor(exec);
        wuow.commit();
    }

    void deregisterExecutor(PlanExecutor* exec) {
        WriteUnitOfWork wuow(&_opCtx);
        _ctx->ctx()
            .db()
            ->getOrCreateCollection(&_opCtx, ns())
            ->getCursorManager()
            ->deregisterExecutor(exec);
        wuow.commit();
    }

    int N() {
        return 50;
    }

    Collection* collection() {
        return _ctx->ctx().db()->getCollection(ns());
    }

    static const char* ns() {
        return "unittests.ExecutorRegistryDiskLocInvalidation";
    }

    // Order of these is important for initialization
    OperationContextImpl _opCtx;
    auto_ptr<Client::WriteContext> _ctx;
    DBDirectClient _client;
};


// Test that a registered runner receives invalidation notifications.
class ExecutorRegistryDiskLocInvalid : public ExecutorRegistryBase {
public:
    void run() {
        if (supportsDocLocking()) {
            return;
        }

        auto_ptr<PlanExecutor> run(getCollscan());
        BSONObj obj;

        // Read some of it.
        for (int i = 0; i < 10; ++i) {
            ASSERT_EQUALS(PlanExecutor::ADVANCED, run->getNext(&obj, NULL));
            ASSERT_EQUALS(i, obj["foo"].numberInt());
        }

        // Register it.
        run->saveState();
        registerExecutor(run.get());
        // At this point it's safe to yield.  forceYield would do that.  Let's now simulate some
        // stuff going on in the yield.

        // Delete some data, namely the next 2 things we'd expect.
        _client.remove(ns(), BSON("foo" << 10));
        _client.remove(ns(), BSON("foo" << 11));

        // At this point, we're done yielding.  We recover our lock.

        // Unregister the runner.
        deregisterExecutor(run.get());

        // And clean up anything that happened before.
        run->restoreState(&_opCtx);

        // Make sure that the runner moved forward over the deleted data.  We don't see foo==10
        // or foo==11.
        for (int i = 12; i < N(); ++i) {
            ASSERT_EQUALS(PlanExecutor::ADVANCED, run->getNext(&obj, NULL));
            ASSERT_EQUALS(i, obj["foo"].numberInt());
        }

        ASSERT_EQUALS(PlanExecutor::IS_EOF, run->getNext(&obj, NULL));
    }
};

// Test that registered runners are killed when their collection is dropped.
class ExecutorRegistryDropCollection : public ExecutorRegistryBase {
public:
    void run() {
        auto_ptr<PlanExecutor> run(getCollscan());
        BSONObj obj;

        // Read some of it.
        for (int i = 0; i < 10; ++i) {
            ASSERT_EQUALS(PlanExecutor::ADVANCED, run->getNext(&obj, NULL));
            ASSERT_EQUALS(i, obj["foo"].numberInt());
        }

        // Save state and register.
        run->saveState();
        registerExecutor(run.get());

        // Drop a collection that's not ours.
        _client.dropCollection("unittests.someboguscollection");

        // Unregister and restore state.
        deregisterExecutor(run.get());
        run->restoreState(&_opCtx);

        ASSERT_EQUALS(PlanExecutor::ADVANCED, run->getNext(&obj, NULL));
        ASSERT_EQUALS(10, obj["foo"].numberInt());

        // Save state and register.
        run->saveState();
        registerExecutor(run.get());

        // Drop our collection.
        _client.dropCollection(ns());

        // Unregister and restore state.
        deregisterExecutor(run.get());
        run->restoreState(&_opCtx);

        // PlanExecutor was killed.
        ASSERT_EQUALS(PlanExecutor::DEAD, run->getNext(&obj, NULL));
    }
};

// Test that registered runners are killed when all indices are dropped on the collection.
class ExecutorRegistryDropAllIndices : public ExecutorRegistryBase {
public:
    void run() {
        auto_ptr<PlanExecutor> run(getCollscan());
        BSONObj obj;

        ASSERT_OK(dbtests::createIndex(&_opCtx, ns(), BSON("foo" << 1)));

        // Read some of it.
        for (int i = 0; i < 10; ++i) {
            ASSERT_EQUALS(PlanExecutor::ADVANCED, run->getNext(&obj, NULL));
            ASSERT_EQUALS(i, obj["foo"].numberInt());
        }

        // Save state and register.
        run->saveState();
        registerExecutor(run.get());

        // Drop all indices.
        _client.dropIndexes(ns());

        // Unregister and restore state.
        deregisterExecutor(run.get());
        run->restoreState(&_opCtx);

        // PlanExecutor was killed.
        ASSERT_EQUALS(PlanExecutor::DEAD, run->getNext(&obj, NULL));
    }
};

// Test that registered runners are killed when an index is dropped on the collection.
class ExecutorRegistryDropOneIndex : public ExecutorRegistryBase {
public:
    void run() {
        auto_ptr<PlanExecutor> run(getCollscan());
        BSONObj obj;

        ASSERT_OK(dbtests::createIndex(&_opCtx, ns(), BSON("foo" << 1)));

        // Read some of it.
        for (int i = 0; i < 10; ++i) {
            ASSERT_EQUALS(PlanExecutor::ADVANCED, run->getNext(&obj, NULL));
            ASSERT_EQUALS(i, obj["foo"].numberInt());
        }

        // Save state and register.
        run->saveState();
        registerExecutor(run.get());

        // Drop a specific index.
        _client.dropIndex(ns(), BSON("foo" << 1));

        // Unregister and restore state.
        deregisterExecutor(run.get());
        run->restoreState(&_opCtx);

        // PlanExecutor was killed.
        ASSERT_EQUALS(PlanExecutor::DEAD, run->getNext(&obj, NULL));
    }
};

// Test that registered runners are killed when their database is dropped.
class ExecutorRegistryDropDatabase : public ExecutorRegistryBase {
public:
    void run() {
        auto_ptr<PlanExecutor> run(getCollscan());
        BSONObj obj;

        // Read some of it.
        for (int i = 0; i < 10; ++i) {
            ASSERT_EQUALS(PlanExecutor::ADVANCED, run->getNext(&obj, NULL));
            ASSERT_EQUALS(i, obj["foo"].numberInt());
        }

        // Save state and register.
        run->saveState();
        registerExecutor(run.get());

        // Drop a DB that's not ours.  We can't have a lock at all to do this as dropping a DB
        // requires a "global write lock."
        _ctx.reset();
        _client.dropDatabase("somesillydb");
        _ctx.reset(new Client::WriteContext(&_opCtx, ns()));

        // Unregister and restore state.
        deregisterExecutor(run.get());
        run->restoreState(&_opCtx);

        ASSERT_EQUALS(PlanExecutor::ADVANCED, run->getNext(&obj, NULL));
        ASSERT_EQUALS(10, obj["foo"].numberInt());

        // Save state and register.
        run->saveState();
        registerExecutor(run.get());

        // Drop our DB.  Once again, must give up the lock.
        _ctx.reset();
        _client.dropDatabase("unittests");
        _ctx.reset(new Client::WriteContext(&_opCtx, ns()));

        // Unregister and restore state.
        deregisterExecutor(run.get());
        run->restoreState(&_opCtx);
        _ctx.reset();

        // PlanExecutor was killed.
        ASSERT_EQUALS(PlanExecutor::DEAD, run->getNext(&obj, NULL));
    }
};

// TODO: Test that this works with renaming a collection.

class All : public Suite {
public:
    All() : Suite("executor_registry") {}

    void setupTests() {
        add<ExecutorRegistryDiskLocInvalid>();
        add<ExecutorRegistryDropCollection>();
        add<ExecutorRegistryDropAllIndices>();
        add<ExecutorRegistryDropOneIndex>();
        add<ExecutorRegistryDropDatabase>();
    }
};

SuiteInstance<All> executorRegistryAll;

}  // namespace ExecutorRegistry
