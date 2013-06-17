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

//#include "mongo/pch.h"

#include "mongo/bson/util/misc.h" // for Date_t
#include "mongo/s/type_lockpings.h"
#include "mongo/unittest/unittest.h"

namespace {

    using std::string;
    using mongo::BSONObj;
    using mongo::LockpingsType;
    using mongo::Date_t;

    TEST(Validity, MissingFields) {
        LockpingsType lockPing;
        BSONObj objNoProcess = BSON(LockpingsType::ping(time(0)));
        string errMsg;
        ASSERT(lockPing.parseBSON(objNoProcess, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_FALSE(lockPing.isValid(NULL));

        BSONObj objNoPing = BSON(LockpingsType::process("host.local:27017:1352918870:16807"));
        ASSERT(lockPing.parseBSON(objNoPing, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_FALSE(lockPing.isValid(NULL));
    }

    TEST(Validity, Valid) {
        LockpingsType lockPing;
        BSONObj obj = BSON(LockpingsType::process("host.local:27017:1352918870:16807") <<
                           LockpingsType::ping(1ULL));
        string errMsg;
        ASSERT(lockPing.parseBSON(obj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_TRUE(lockPing.isValid(NULL));
        ASSERT_EQUALS(lockPing.getProcess(), "host.local:27017:1352918870:16807");
        ASSERT_EQUALS(lockPing.getPing(), 1ULL);
    }

    TEST(Validity, BadType) {
        LockpingsType lockPing;
        BSONObj obj = BSON(LockpingsType::process() << 0);
        string errMsg;
        ASSERT((!lockPing.parseBSON(obj, &errMsg)) && (errMsg != ""));
    }

} // unnamed namespace
