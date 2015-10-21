/*    Copyright 2012 10gen Inc.
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

#include "mongo/db/jsobj.h"
#include "mongo/unittest/unittest.h"

namespace {

using mongo::BSONField;
using mongo::BSONObj;

TEST(Assignment, Simple) {
    BSONField<int> x("x");
    BSONObj o = BSON(x << 5);
    ASSERT_EQUALS(BSON("x" << 5), o);
}

TEST(Make, Simple) {
    BSONField<int> x("x");
    BSONObj o = BSON(x.make(5));
    ASSERT_EQUALS(BSON("x" << 5), o);
}

TEST(Query, GreaterThan) {
    BSONField<int> x("x");
    BSONObj o = BSON(x(5));
    ASSERT_EQUALS(BSON("x" << 5), o);

    o = BSON(x.gt(5));
    ASSERT_EQUALS(BSON("x" << BSON("$gt" << 5)), o);
}

TEST(Query, NotEqual) {
    BSONField<int> x("x");
    BSONObj o = BSON(x(10));
    ASSERT_EQUALS(BSON("x" << 10), o);

    o = BSON(x.ne(5));
    ASSERT_EQUALS(BSON("x" << BSON("$ne" << 5)), o);
}

}  // unnamed namespace
