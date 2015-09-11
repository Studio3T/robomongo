// fts_matcher_test.cpp

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

#include "mongo/platform/basic.h"

#include "mongo/db/fts/fts_matcher.h"
#include "mongo/unittest/unittest.h"

namespace mongo {
namespace fts {

TEST(FTSMatcher, NegWild1) {
    FTSQuery q;
    ASSERT_OK(q.parse("foo -bar", "english", TEXT_INDEX_VERSION_2));
    FTSMatcher m(q,
                 FTSSpec(FTSSpec::fixSpec(BSON("key" << BSON("$**"
                                                             << "text")))));

    ASSERT(m.hasNegativeTerm(BSON("x" << BSON("y"
                                              << "bar"))));
    ASSERT(m.hasNegativeTerm(BSON("x" << BSON("y"
                                              << "bar"))));
}

// Regression test for SERVER-11994.
TEST(FTSMatcher, NegWild2) {
    FTSQuery q;
    ASSERT_OK(q.parse("pizza -restaurant", "english", TEXT_INDEX_VERSION_2));
    FTSMatcher m(q,
                 FTSSpec(FTSSpec::fixSpec(BSON("key" << BSON("$**"
                                                             << "text")))));

    ASSERT(m.hasNegativeTerm(BSON("x" << BSON("y"
                                              << "pizza restaurant"))));
    ASSERT(m.hasNegativeTerm(BSON("x" << BSON("y"
                                              << "PIZZA RESTAURANT"))));
}

TEST(FTSMatcher, Phrase1) {
    FTSQuery q;
    ASSERT_OK(q.parse("foo \"table top\"", "english", TEXT_INDEX_VERSION_2));
    FTSMatcher m(q,
                 FTSSpec(FTSSpec::fixSpec(BSON("key" << BSON("$**"
                                                             << "text")))));

    ASSERT(m.phraseMatch("table top",
                         BSON("x"
                              << "table top")));
    ASSERT(m.phraseMatch("table top",
                         BSON("x"
                              << " asd table top asd")));
    ASSERT(!m.phraseMatch("table top",
                          BSON("x"
                               << "tablz top")));
    ASSERT(!m.phraseMatch("table top",
                          BSON("x"
                               << " asd tablz top asd")));

    ASSERT(m.phrasesMatch(BSON("x"
                               << "table top")));
    ASSERT(!m.phrasesMatch(BSON("x"
                                << "table a top")));
}

TEST(FTSMatcher, Phrase2) {
    FTSQuery q;
    ASSERT_OK(q.parse("foo \"table top\"", "english", TEXT_INDEX_VERSION_2));
    FTSMatcher m(q,
                 FTSSpec(FTSSpec::fixSpec(BSON("key" << BSON("x"
                                                             << "text")))));
    ASSERT(m.phraseMatch("table top", BSON("x" << BSON_ARRAY("table top"))));
}

// Test that the matcher parses the document with the document language, not the search
// language.
TEST(FTSMatcher, ParsesUsingDocLanguage) {
    FTSQuery q;
    ASSERT_OK(q.parse("-glad", "none", TEXT_INDEX_VERSION_2));
    FTSMatcher m(q,
                 FTSSpec(FTSSpec::fixSpec(BSON("key" << BSON("x"
                                                             << "text")))));

    // Even though the search language is "none", the document {x: "gladly"} should be
    // parsed using the English stemmer, and as such should match the negated term "glad".
    ASSERT(m.hasNegativeTerm(BSON("x"
                                  << "gladly")));
}
}
}
