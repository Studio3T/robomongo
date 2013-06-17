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

#include "mongo/s/type_tags.h"
#include "mongo/unittest/unittest.h"

namespace {

    using std::string;
    using mongo::BSONObj;
    using mongo::TagsType;

    TEST(Validity, Valid) {
        TagsType tag;
        BSONObj obj = BSON(TagsType::ns("test.mycol") <<
                           TagsType::tag("tag") <<
                           TagsType::min(BSON("a" << 10)) <<
                           TagsType::max(BSON("a" << 20)));
        string errMsg;
        ASSERT(tag.parseBSON(obj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_TRUE(tag.isValid(NULL));
        ASSERT_EQUALS(tag.getNS(), "test.mycol");
        ASSERT_EQUALS(tag.getTag(), "tag");
        ASSERT_EQUALS(tag.getMin(), BSON("a" << 10));
        ASSERT_EQUALS(tag.getMax(), BSON("a" << 20));
    }

    TEST(Validity, MissingFields) {
        TagsType tag;
        BSONObj objModNS = BSON(TagsType::tag("tag") <<
                                TagsType::min(BSON("a" << 10)) <<
                                TagsType::max(BSON("a" << 20)));
        string errMsg;
        ASSERT(tag.parseBSON(objModNS, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_FALSE(tag.isValid(NULL));

        BSONObj objModName = BSON(TagsType::ns("test.mycol") <<
                                  TagsType::min(BSON("a" << 10)) <<
                                  TagsType::max(BSON("a" << 20)));
        ASSERT(tag.parseBSON(objModName, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_FALSE(tag.isValid(NULL));

        BSONObj objModKeys = BSON(TagsType::ns("test.mycol") <<
                                  TagsType::tag("tag"));
        ASSERT(tag.parseBSON(objModKeys, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_FALSE(tag.isValid(NULL));
    }

    TEST(MinMaxValidity, DifferentNumberOfColumns) {
        TagsType tag;
        BSONObj obj = BSON(TagsType::tag("tag") <<
                           TagsType::ns("test.mycol") <<
                           TagsType::min(BSON("a" << 10 << "b" << 10)) <<
                           TagsType::max(BSON("a" << 20)));
        string errMsg;
        ASSERT(tag.parseBSON(obj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_FALSE(tag.isValid(NULL));
    }

    TEST(MinMaxValidity, DifferentColumns) {
        TagsType tag;
        BSONObj obj = BSON(TagsType::tag("tag") <<
                           TagsType::ns("test.mycol") <<
                           TagsType::min(BSON("a" << 10)) <<
                           TagsType::max(BSON("b" << 20)));
        string errMsg;
        ASSERT(tag.parseBSON(obj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_FALSE(tag.isValid(NULL));
    }

    TEST(MinMaxValidity, NotAscending) {
        TagsType tag;
        BSONObj obj = BSON(TagsType::tag("tag") <<
                           TagsType::ns("test.mycol") <<
                           TagsType::min(BSON("a" << 20)) <<
                           TagsType::max(BSON("a" << 10)));
        string errMsg;
        ASSERT(tag.parseBSON(obj, &errMsg));
        ASSERT_EQUALS(errMsg, "");
        ASSERT_FALSE(tag.isValid(NULL));
    }

    TEST(Validity, BadType) {
        TagsType tag;
        BSONObj obj = BSON(TagsType::tag() << 0);
        string errMsg;
        ASSERT((!tag.parseBSON(obj, &errMsg)) && (errMsg != ""));
    }

} // unnamed namespace
