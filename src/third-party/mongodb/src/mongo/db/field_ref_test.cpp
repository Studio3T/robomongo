/*    Copyright 2012 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <string>

#include "mongo/base/error_codes.h"
#include "mongo/base/status.h"
#include "mongo/base/string_data.h"
#include "mongo/db/field_ref.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/mongoutils/str.h"

namespace {

    using mongo::FieldRef;
    using mongo::StringData;
    using mongoutils::str::stream;
    using std::string;

    TEST(Empty, NoFields) {
        FieldRef fieldRef;
        fieldRef.parse("");
        ASSERT_EQUALS(fieldRef.numParts(), 0U);
        ASSERT_EQUALS(fieldRef.dottedField(), "");
    }

    TEST(Empty, NoFieldNames) {
        string field = ".";
        FieldRef fieldRef;
        fieldRef.parse(field);
        ASSERT_EQUALS(fieldRef.numParts(), 2U);
        ASSERT_EQUALS(fieldRef.getPart(0), "");
        ASSERT_EQUALS(fieldRef.getPart(1), "");
        ASSERT_EQUALS(fieldRef.dottedField(), field);
    }

    TEST(Empty, EmptyFieldName) {
        string field = ".b.";
        FieldRef fieldRef;
        fieldRef.parse(field);
        ASSERT_EQUALS(fieldRef.numParts(), 3U);
        ASSERT_EQUALS(fieldRef.getPart(0), "");
        ASSERT_EQUALS(fieldRef.getPart(1), "b");
        ASSERT_EQUALS(fieldRef.getPart(2), "");
        ASSERT_EQUALS(fieldRef.dottedField(), field);
    }

    TEST(Normal, SinglePart) {
        string field = "a";
        FieldRef fieldRef;
        fieldRef.parse(field);
        ASSERT_EQUALS(fieldRef.numParts(), 1U);
        ASSERT_EQUALS(fieldRef.getPart(0), field);
        ASSERT_EQUALS(fieldRef.dottedField(), field);
    }

    TEST(Normal, MulitplePartsVariable) {
        const char* parts[] = {"a", "b", "c", "d", "e"};
        size_t size = sizeof(parts)/sizeof(char*);
        string field(parts[0]);
        for (size_t i=1; i<size; i++) {
            field.append(1, '.');
            field.append(parts[i]);
        }

        FieldRef fieldRef;
        fieldRef.parse(field);
        ASSERT_EQUALS(fieldRef.numParts(), size);
        for (size_t i=0; i<size; i++) {
            ASSERT_EQUALS(fieldRef.getPart(i), parts[i]);
        }
        ASSERT_EQUALS(fieldRef.dottedField(), field);
    }

    TEST(Replacement, SingleField) {
        string field = "$";
        FieldRef fieldRef;
        fieldRef.parse(field);
        ASSERT_EQUALS(fieldRef.numParts(), 1U);
        ASSERT_EQUALS(fieldRef.getPart(0), "$");

        string newField = "a";
        fieldRef.setPart(0, newField);
        ASSERT_EQUALS(fieldRef.numParts(), 1U);
        ASSERT_EQUALS(fieldRef.getPart(0), newField);
        ASSERT_EQUALS(fieldRef.dottedField(), newField);
    }

    TEST(Replacement, InMultipleField) {
        string field = "a.b.c.$.e";
        FieldRef fieldRef;
        fieldRef.parse(field);
        ASSERT_EQUALS(fieldRef.numParts(), 5U);
        ASSERT_EQUALS(fieldRef.getPart(3), "$");

        string newField = "d";
        fieldRef.setPart(3, newField);
        ASSERT_EQUALS(fieldRef.numParts(), 5U);
        ASSERT_EQUALS(fieldRef.getPart(3), newField);
        ASSERT_EQUALS(fieldRef.dottedField(), "a.b.c.d.e");
    }

    TEST(Replacement, SameFieldMultipleReplacements) {
        string prefix = "a.";
        string field = prefix + "$";
        FieldRef fieldRef;
        fieldRef.parse(field);
        ASSERT_EQUALS(fieldRef.numParts(), 2U);

        const char* parts[] = {"a", "b", "c", "d", "e"};
        size_t size = sizeof(parts)/sizeof(char*);
        for (size_t i=0; i<size; i++) {
            fieldRef.setPart(1, parts[i]);
            ASSERT_EQUALS(fieldRef.dottedField(), prefix + parts[i]);
        }
        ASSERT_EQUALS(fieldRef.numReplaced(), 1U);
    }

} // namespace mongo
