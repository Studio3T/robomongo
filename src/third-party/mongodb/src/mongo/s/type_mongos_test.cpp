/**
 *    Copyright (C) 2012 10gen Inc.
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
 */

#include "mongo/bson/util/misc.h" // for Date_t
#include "mongo/s/type_mongos.h"
#include "mongo/unittest/unittest.h"

namespace {

    using std::string;
    using mongo::BSONObj;
    using mongo::MongosType;
    using mongo::Date_t;

    TEST(Validity, MissingName) {
        MongosType mongos;
        string errMsg;
        BSONObj obj = BSON(MongosType::ping(1ULL) <<
                           MongosType::up(100) <<
                           MongosType::waiting(false) <<
                           MongosType::mongoVersion("x.x.x") <<
                           MongosType::configVersion(0));
        ASSERT(mongos.parseBSON(obj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_FALSE(mongos.isNameSet());
        ASSERT_TRUE(mongos.isPingSet());
        ASSERT_TRUE(mongos.isUpSet());
        ASSERT_TRUE(mongos.isWaitingSet());
        ASSERT_TRUE(mongos.isMongoVersionSet());
        ASSERT_TRUE(mongos.isConfigVersionSet());
        ASSERT_FALSE(mongos.isValid(NULL));
    }

    TEST(Validity, MissingPing) {
        MongosType mongos;
        string errMsg;
        BSONObj obj = BSON(MongosType::name("localhost:27017") <<
                           MongosType::up(100) <<
                           MongosType::waiting(false) <<
                           MongosType::mongoVersion("x.x.x") <<
                           MongosType::configVersion(0));
        ASSERT(mongos.parseBSON(obj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_TRUE(mongos.isNameSet());
        ASSERT_FALSE(mongos.isPingSet());
        ASSERT_TRUE(mongos.isUpSet());
        ASSERT_TRUE(mongos.isWaitingSet());
        ASSERT_TRUE(mongos.isMongoVersionSet());
        ASSERT_TRUE(mongos.isConfigVersionSet());
        ASSERT_FALSE(mongos.isValid(NULL));
    }

    TEST(Validity, MissingUp) {
        MongosType mongos;
        string errMsg;
        BSONObj obj = BSON(MongosType::name("localhost:27017") <<
                           MongosType::ping(1ULL) <<
                           MongosType::waiting(false) <<
                           MongosType::mongoVersion("x.x.x") <<
                           MongosType::configVersion(0));
        ASSERT(mongos.parseBSON(obj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_TRUE(mongos.isNameSet());
        ASSERT_TRUE(mongos.isPingSet());
        ASSERT_FALSE(mongos.isUpSet());
        ASSERT_TRUE(mongos.isWaitingSet());
        ASSERT_TRUE(mongos.isMongoVersionSet());
        ASSERT_TRUE(mongos.isConfigVersionSet());
        ASSERT_FALSE(mongos.isValid(NULL));
    }

    TEST(Validity, MissingWaiting) {
        MongosType mongos;
        string errMsg;
        BSONObj obj = BSON(MongosType::name("localhost:27017") <<
                           MongosType::ping(1ULL) <<
                           MongosType::up(100) <<
                           MongosType::mongoVersion("x.x.x") <<
                           MongosType::configVersion(0));
        ASSERT(mongos.parseBSON(obj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_TRUE(mongos.isNameSet());
        ASSERT_TRUE(mongos.isPingSet());
        ASSERT_TRUE(mongos.isUpSet());
        ASSERT_FALSE(mongos.isWaitingSet());
        ASSERT_TRUE(mongos.isMongoVersionSet());
        ASSERT_TRUE(mongos.isConfigVersionSet());
        ASSERT_FALSE(mongos.isValid(NULL));
    }

    TEST(Validity, MissingMongoVersion) {
        MongosType mongos;
        string errMsg;
        BSONObj obj = BSON(MongosType::name("localhost:27017") <<
                           MongosType::ping(1ULL) <<
                           MongosType::up(100) <<
                           MongosType::waiting(false) <<
                           MongosType::configVersion(0));
        ASSERT(mongos.parseBSON(obj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_TRUE(mongos.isNameSet());
        ASSERT_TRUE(mongos.isPingSet());
        ASSERT_TRUE(mongos.isUpSet());
        ASSERT_TRUE(mongos.isWaitingSet());
        ASSERT_FALSE(mongos.isMongoVersionSet());
        ASSERT_TRUE(mongos.isConfigVersionSet());
        /* NOTE: mongoVersion should eventually become mandatory, but is optional now for backward
         * compatibility reasons */
        ASSERT_TRUE(mongos.isValid(NULL));
    }

    TEST(Validity, MissingConfigVersion) {
        MongosType mongos;
        string errMsg;
        BSONObj obj = BSON(MongosType::name("localhost:27017") <<
                           MongosType::ping(1ULL) <<
                           MongosType::up(100) <<
                           MongosType::waiting(false) <<
                           MongosType::mongoVersion("x.x.x"));
        ASSERT(mongos.parseBSON(obj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_TRUE(mongos.isNameSet());
        ASSERT_TRUE(mongos.isPingSet());
        ASSERT_TRUE(mongos.isUpSet());
        ASSERT_TRUE(mongos.isWaitingSet());
        ASSERT_TRUE(mongos.isMongoVersionSet());
        ASSERT_FALSE(mongos.isConfigVersionSet());
        /* NOTE: configVersion should eventually become mandatory, but is optional now for backward
         * compatibility reasons */
        ASSERT_TRUE(mongos.isValid(NULL));
    }

    TEST(Validity, Valid) {
        MongosType mongos;
        BSONObj obj = BSON(MongosType::name("localhost:27017") <<
                           MongosType::ping(1ULL) <<
                           MongosType::up(100) <<
                           MongosType::waiting(false) <<
                           MongosType::mongoVersion("x.x.x") <<
                           MongosType::configVersion(0));
        string errMsg;
        ASSERT(mongos.parseBSON(obj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_TRUE(mongos.isValid(NULL));
        ASSERT_EQUALS(mongos.getName(), "localhost:27017");
        ASSERT_EQUALS(mongos.getPing(), 1ULL);
        ASSERT_EQUALS(mongos.getUp(), 100);
        ASSERT_EQUALS(mongos.getWaiting(), false);
        ASSERT_EQUALS(mongos.getMongoVersion(), "x.x.x");
        ASSERT_EQUALS(mongos.getConfigVersion(), 0);
    }

    TEST(Validity, BadType) {
        MongosType mongos;
        BSONObj obj = BSON(MongosType::name() << 0);
        string errMsg;
        ASSERT((!mongos.parseBSON(obj, &errMsg)) && (errMsg != ""));
    }

} // unnamed namespace
