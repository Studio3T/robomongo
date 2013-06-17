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
#include "mongo/s/type_changelog.h"
#include "mongo/unittest/unittest.h"

namespace {

    using std::string;
    using mongo::ChangelogType;
    using mongo::BSONObj;
    using mongo::Date_t;

    TEST(Validity, Empty) {
        ChangelogType logEntry;
        BSONObj emptyObj = BSONObj();
        string errMsg;
        ASSERT(logEntry.parseBSON(emptyObj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_FALSE(logEntry.isValid(NULL));
    }

    TEST(Validity, Valid) {
        ChangelogType logEntry;
        BSONObj obj = BSON(ChangelogType::changeID("host.local-2012-11-21T19:14:10-8") <<
                           ChangelogType::server("host.local") <<
                           ChangelogType::clientAddr("192.168.0.189:51128") <<
                           ChangelogType::time(1ULL) <<
                           ChangelogType::what("split") <<
                           ChangelogType::ns("test.test") <<
                           ChangelogType::details(BSON("dummy" << "info")));
        string errMsg;
        ASSERT(logEntry.parseBSON(obj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_TRUE(logEntry.isValid(NULL));
        ASSERT_EQUALS(logEntry.getChangeID(), "host.local-2012-11-21T19:14:10-8");
        ASSERT_EQUALS(logEntry.getServer(), "host.local");
        ASSERT_EQUALS(logEntry.getClientAddr(), "192.168.0.189:51128");
        ASSERT_EQUALS(logEntry.getTime(), 1ULL);
        ASSERT_EQUALS(logEntry.getWhat(), "split");
        ASSERT_EQUALS(logEntry.getNS(), "test.test");
        ASSERT_EQUALS(logEntry.getDetails(), BSON("dummy" << "info"));
    }

    TEST(Validity, MissingChangeID) {
        ChangelogType logEntry;
        BSONObj obj = BSON(ChangelogType::server("host.local") <<
                           ChangelogType::clientAddr("192.168.0.189:51128") <<
                           ChangelogType::time(1ULL) <<
                           ChangelogType::what("split") <<
                           ChangelogType::ns("test.test") <<
                           ChangelogType::details(BSON("dummy" << "info")));
        string errMsg;
        ASSERT(logEntry.parseBSON(obj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_FALSE(logEntry.isValid(NULL));
    }

    TEST(Validity, MissingServer) {
        ChangelogType logEntry;
        BSONObj obj = BSON(ChangelogType::changeID("host.local-2012-11-21T19:14:10-8") <<
                           ChangelogType::clientAddr("192.168.0.189:51128") <<
                           ChangelogType::time(1ULL) <<
                           ChangelogType::what("split") <<
                           ChangelogType::ns("test.test") <<
                           ChangelogType::details(BSON("dummy" << "info")));
        string errMsg;
        ASSERT(logEntry.parseBSON(obj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_FALSE(logEntry.isValid(NULL));
    }

    TEST(Validity, MissingClientAddr) {
        ChangelogType logEntry;
        BSONObj obj = BSON(ChangelogType::changeID("host.local-2012-11-21T19:14:10-8") <<
                           ChangelogType::server("host.local") <<
                           ChangelogType::time(1ULL) <<
                           ChangelogType::what("split") <<
                           ChangelogType::ns("test.test") <<
                           ChangelogType::details(BSON("dummy" << "info")));
        string errMsg;
        ASSERT(logEntry.parseBSON(obj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_FALSE(logEntry.isValid(NULL));
    }

    TEST(Validity, MissingTime) {
        ChangelogType logEntry;
        BSONObj obj = BSON(ChangelogType::changeID("host.local-2012-11-21T19:14:10-8") <<
                           ChangelogType::server("host.local") <<
                           ChangelogType::clientAddr("192.168.0.189:51128") <<
                           ChangelogType::what("split") <<
                           ChangelogType::ns("test.test") <<
                           ChangelogType::details(BSON("dummy" << "info")));
        string errMsg;
        ASSERT(logEntry.parseBSON(obj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_FALSE(logEntry.isValid(NULL));
    }

    TEST(Validity, MissingWhat) {
        ChangelogType logEntry;
        BSONObj obj = BSON(ChangelogType::changeID("host.local-2012-11-21T19:14:10-8") <<
                           ChangelogType::server("host.local") <<
                           ChangelogType::clientAddr("192.168.0.189:51128") <<
                           ChangelogType::time(1ULL) <<
                           ChangelogType::ns("test.test") <<
                           ChangelogType::details(BSON("dummy" << "info")));
        string errMsg;
        ASSERT(logEntry.parseBSON(obj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_FALSE(logEntry.isValid(NULL));
    }

    TEST(Validity, MissingNS) {
        ChangelogType logEntry;
        BSONObj obj = BSON(ChangelogType::changeID("host.local-2012-11-21T19:14:10-8") <<
                           ChangelogType::server("host.local") <<
                           ChangelogType::clientAddr("192.168.0.189:51128") <<
                           ChangelogType::time(1ULL) <<
                           ChangelogType::what("split") <<
                           ChangelogType::details(BSON("dummy" << "info")));
        string errMsg;
        ASSERT(logEntry.parseBSON(obj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_FALSE(logEntry.isValid(NULL));
    }

    TEST(Validity, MissingDetails) {
        ChangelogType logEntry;
        BSONObj obj = BSON(ChangelogType::changeID("host.local-2012-11-21T19:14:10-8") <<
                           ChangelogType::server("host.local") <<
                           ChangelogType::clientAddr("192.168.0.189:51128") <<
                           ChangelogType::time(1ULL) <<
                           ChangelogType::what("split") <<
                           ChangelogType::ns("test.test"));
        string errMsg;
        ASSERT(logEntry.parseBSON(obj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_FALSE(logEntry.isValid(NULL));
    }

    TEST(Validity, BadType) {
        ChangelogType logEntry;
        BSONObj obj = BSON(ChangelogType::changeID() << 0);
        string errMsg;
        ASSERT((!logEntry.parseBSON(obj, &errMsg)) && (errMsg != ""));
    }

} // unnamed namespace
