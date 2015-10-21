// client.cpp

/*
 *    Copyright (C) 2010 10gen Inc.
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

#include "mongo/client/dbclientcursor.h"
#include "mongo/db/catalog/collection.h"
#include "mongo/db/catalog/database.h"
#include "mongo/db/dbdirectclient.h"
#include "mongo/db/operation_context_impl.h"
#include "mongo/dbtests/dbtests.h"


namespace ClientTests {

using std::auto_ptr;
using std::string;
using std::vector;

class Base {
public:
    Base(string coll) : _ns("test." + coll) {
        OperationContextImpl txn;
        DBDirectClient db(&txn);

        db.dropDatabase("test");
    }

    virtual ~Base() {
        OperationContextImpl txn;
        DBDirectClient db(&txn);

        db.dropCollection(_ns);
    }

    const char* ns() {
        return _ns.c_str();
    }

    const string _ns;
};


class DropIndex : public Base {
public:
    DropIndex() : Base("dropindex") {}
    void run() {
        OperationContextImpl txn;
        DBDirectClient db(&txn);

        db.insert(ns(), BSON("x" << 2));
        ASSERT_EQUALS(1u, db.getIndexSpecs(ns()).size());

        ASSERT_OK(dbtests::createIndex(&txn, ns(), BSON("x" << 1)));
        ASSERT_EQUALS(2u, db.getIndexSpecs(ns()).size());

        db.dropIndex(ns(), BSON("x" << 1));
        ASSERT_EQUALS(1u, db.getIndexSpecs(ns()).size());

        ASSERT_OK(dbtests::createIndex(&txn, ns(), BSON("x" << 1)));
        ASSERT_EQUALS(2u, db.getIndexSpecs(ns()).size());

        db.dropIndexes(ns());
        ASSERT_EQUALS(1u, db.getIndexSpecs(ns()).size());
    }
};

/**
 * Check that nIndexes is incremented correctly when an index builds (and that it is not
 * incremented when an index fails to build), system.indexes has an entry added (or not), and
 * system.namespaces has a doc added (or not).
 */
class BuildIndex : public Base {
public:
    BuildIndex() : Base("buildIndex") {}
    void run() {
        OperationContextImpl txn;

        Client::WriteContext ctx(&txn, ns());
        DBDirectClient db(&txn);

        db.insert(ns(), BSON("x" << 1 << "y" << 2));
        db.insert(ns(), BSON("x" << 2 << "y" << 2));

        Collection* collection = ctx.getCollection();
        ASSERT(collection);
        IndexCatalog* indexCatalog = collection->getIndexCatalog();

        ASSERT_EQUALS(1, indexCatalog->numIndexesReady(&txn));
        // _id index
        ASSERT_EQUALS(1U, db.getIndexSpecs(ns()).size());

        ASSERT_EQUALS(ErrorCodes::DuplicateKey,
                      dbtests::createIndex(&txn, ns(), BSON("y" << 1), true));

        ASSERT_EQUALS(1, indexCatalog->numIndexesReady(&txn));
        ASSERT_EQUALS(1U, db.getIndexSpecs(ns()).size());

        ASSERT_OK(dbtests::createIndex(&txn, ns(), BSON("x" << 1), true));

        ASSERT_EQUALS(2, indexCatalog->numIndexesReady(&txn));
        ASSERT_EQUALS(2U, db.getIndexSpecs(ns()).size());
    }
};

class CS_10 : public Base {
public:
    CS_10() : Base("CS_10") {}
    void run() {
        OperationContextImpl txn;
        DBDirectClient db(&txn);

        const string longs(770, 'c');
        for (int i = 0; i < 1111; ++i) {
            db.insert(ns(), BSON("a" << i << "b" << longs));
        }

        ASSERT_OK(dbtests::createIndex(&txn, ns(), BSON("a" << 1 << "b" << 1)));

        auto_ptr<DBClientCursor> c = db.query(ns(), Query().sort(BSON("a" << 1 << "b" << 1)));
        ASSERT_EQUALS(1111, c->itcount());
    }
};

class PushBack : public Base {
public:
    PushBack() : Base("PushBack") {}
    void run() {
        OperationContextImpl txn;
        DBDirectClient db(&txn);

        for (int i = 0; i < 10; ++i) {
            db.insert(ns(), BSON("i" << i));
        }

        auto_ptr<DBClientCursor> c = db.query(ns(), Query().sort(BSON("i" << 1)));

        BSONObj o = c->next();
        ASSERT(c->more());
        ASSERT_EQUALS(9, c->objsLeftInBatch());
        ASSERT(c->moreInCurrentBatch());

        c->putBack(o);
        ASSERT(c->more());
        ASSERT_EQUALS(10, c->objsLeftInBatch());
        ASSERT(c->moreInCurrentBatch());

        o = c->next();
        BSONObj o2 = c->next();
        BSONObj o3 = c->next();
        c->putBack(o3);
        c->putBack(o2);
        c->putBack(o);
        for (int i = 0; i < 10; ++i) {
            o = c->next();
            ASSERT_EQUALS(i, o["i"].number());
        }
        ASSERT(!c->more());
        ASSERT_EQUALS(0, c->objsLeftInBatch());
        ASSERT(!c->moreInCurrentBatch());

        c->putBack(o);
        ASSERT(c->more());
        ASSERT_EQUALS(1, c->objsLeftInBatch());
        ASSERT(c->moreInCurrentBatch());
        ASSERT_EQUALS(1, c->itcount());
    }
};

class Create : public Base {
public:
    Create() : Base("Create") {}
    void run() {
        OperationContextImpl txn;
        DBDirectClient db(&txn);

        db.createCollection("unittests.clienttests.create", 4096, true);
        BSONObj info;
        ASSERT(db.runCommand("unittests",
                             BSON("collstats"
                                  << "clienttests.create"),
                             info));
    }
};

class ConnectionStringTests {
public:
    void run() {
        {
            ConnectionString s("a/b,c,d", ConnectionString::SET);
            ASSERT_EQUALS(ConnectionString::SET, s.type());
            ASSERT_EQUALS("a", s.getSetName());
            vector<HostAndPort> v = s.getServers();
            ASSERT_EQUALS(3U, v.size());
            ASSERT_EQUALS("b", v[0].host());
            ASSERT_EQUALS("c", v[1].host());
            ASSERT_EQUALS("d", v[2].host());
        }
    }
};

class All : public Suite {
public:
    All() : Suite("client") {}

    void setupTests() {
        add<DropIndex>();
        add<BuildIndex>();
        add<CS_10>();
        add<PushBack>();
        add<Create>();
        add<ConnectionStringTests>();
    }
};

SuiteInstance<All> all;
}
