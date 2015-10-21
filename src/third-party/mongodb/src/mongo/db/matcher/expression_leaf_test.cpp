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

/** Unit tests for MatchMatchExpression operator implementations in match_operators.{h,cpp}. */

#include "mongo/unittest/unittest.h"

#include "mongo/db/jsobj.h"
#include "mongo/db/json.h"
#include "mongo/db/matcher/expression_parser.h"
#include "mongo/db/matcher/expression.h"
#include "mongo/db/matcher/expression_leaf.h"

namespace mongo {

using std::string;

TEST(EqOp, MatchesElement) {
    BSONObj operand = BSON("a" << 5);
    BSONObj match = BSON("a" << 5.0);
    BSONObj notMatch = BSON("a" << 6);

    EqualityMatchExpression eq;
    eq.init("", operand["a"]);
    ASSERT(eq.matchesSingleElement(match.firstElement()));
    ASSERT(!eq.matchesSingleElement(notMatch.firstElement()));

    ASSERT(eq.equivalent(&eq));
}

TEST(EqOp, InvalidEooOperand) {
    BSONObj operand;
    EqualityMatchExpression eq;
    ASSERT(!eq.init("", operand.firstElement()).isOK());
}

TEST(EqOp, MatchesScalar) {
    BSONObj operand = BSON("a" << 5);
    EqualityMatchExpression eq;
    eq.init("a", operand["a"]);
    ASSERT(eq.matchesBSON(BSON("a" << 5.0), NULL));
    ASSERT(!eq.matchesBSON(BSON("a" << 4), NULL));
}

TEST(EqOp, MatchesArrayValue) {
    BSONObj operand = BSON("a" << 5);
    EqualityMatchExpression eq;
    eq.init("a", operand["a"]);
    ASSERT(eq.matchesBSON(BSON("a" << BSON_ARRAY(5.0 << 6)), NULL));
    ASSERT(!eq.matchesBSON(BSON("a" << BSON_ARRAY(6 << 7)), NULL));
}

TEST(EqOp, MatchesReferencedObjectValue) {
    BSONObj operand = BSON("a.b" << 5);
    EqualityMatchExpression eq;
    eq.init("a.b", operand["a.b"]);
    ASSERT(eq.matchesBSON(BSON("a" << BSON("b" << 5)), NULL));
    ASSERT(eq.matchesBSON(BSON("a" << BSON("b" << BSON_ARRAY(5))), NULL));
    ASSERT(eq.matchesBSON(BSON("a" << BSON_ARRAY(BSON("b" << 5))), NULL));
}

TEST(EqOp, MatchesReferencedArrayValue) {
    BSONObj operand = BSON("a.0" << 5);
    EqualityMatchExpression eq;
    eq.init("a.0", operand["a.0"]);
    ASSERT(eq.matchesBSON(BSON("a" << BSON_ARRAY(5)), NULL));
    ASSERT(!eq.matchesBSON(BSON("a" << BSON_ARRAY(BSON_ARRAY(5))), NULL));
}

TEST(EqOp, MatchesNull) {
    BSONObj operand = BSON("a" << BSONNULL);
    EqualityMatchExpression eq;
    eq.init("a", operand["a"]);
    ASSERT(eq.matchesBSON(BSONObj(), NULL));
    ASSERT(eq.matchesBSON(BSON("a" << BSONNULL), NULL));
    ASSERT(!eq.matchesBSON(BSON("a" << 4), NULL));
    // A non-existent field is treated same way as an empty bson object
    ASSERT(eq.matchesBSON(BSON("b" << 4), NULL));
}

// This test documents how the matcher currently works,
// not necessarily how it should work ideally.
TEST(EqOp, MatchesNestedNull) {
    BSONObj operand = BSON("a.b" << BSONNULL);
    EqualityMatchExpression eq;
    eq.init("a.b", operand["a.b"]);
    // null matches any empty object that is on a subpath of a.b
    ASSERT(eq.matchesBSON(BSONObj(), NULL));
    ASSERT(eq.matchesBSON(BSON("a" << BSONObj()), NULL));
    ASSERT(eq.matchesBSON(BSON("a" << BSON_ARRAY(BSONObj())), NULL));
    ASSERT(eq.matchesBSON(BSON("a" << BSON("b" << BSONNULL)), NULL));
    // b does not exist as an element in array under a.
    ASSERT(!eq.matchesBSON(BSON("a" << BSONArray()), NULL));
    ASSERT(!eq.matchesBSON(BSON("a" << BSON_ARRAY(BSONNULL)), NULL));
    ASSERT(!eq.matchesBSON(BSON("a" << BSON_ARRAY(1 << 2)), NULL));
    // a.b exists but is not null.
    ASSERT(!eq.matchesBSON(BSON("a" << BSON("b" << 4)), NULL));
    ASSERT(!eq.matchesBSON(BSON("a" << BSON("b" << BSONObj())), NULL));
    // A non-existent field is treated same way as an empty bson object
    ASSERT(eq.matchesBSON(BSON("b" << 4), NULL));
}

TEST(EqOp, MatchesMinKey) {
    BSONObj operand = BSON("a" << MinKey);
    EqualityMatchExpression eq;
    eq.init("a", operand["a"]);
    ASSERT(eq.matchesBSON(BSON("a" << MinKey), NULL));
    ASSERT(!eq.matchesBSON(BSON("a" << MaxKey), NULL));
    ASSERT(!eq.matchesBSON(BSON("a" << 4), NULL));
}


TEST(EqOp, MatchesMaxKey) {
    BSONObj operand = BSON("a" << MaxKey);
    EqualityMatchExpression eq;
    ASSERT(eq.init("a", operand["a"]).isOK());
    ASSERT(eq.matchesBSON(BSON("a" << MaxKey), NULL));
    ASSERT(!eq.matchesBSON(BSON("a" << MinKey), NULL));
    ASSERT(!eq.matchesBSON(BSON("a" << 4), NULL));
}

TEST(EqOp, MatchesFullArray) {
    BSONObj operand = BSON("a" << BSON_ARRAY(1 << 2));
    EqualityMatchExpression eq;
    ASSERT(eq.init("a", operand["a"]).isOK());
    ASSERT(eq.matchesBSON(BSON("a" << BSON_ARRAY(1 << 2)), NULL));
    ASSERT(!eq.matchesBSON(BSON("a" << BSON_ARRAY(1 << 2 << 3)), NULL));
    ASSERT(!eq.matchesBSON(BSON("a" << BSON_ARRAY(1)), NULL));
    ASSERT(!eq.matchesBSON(BSON("a" << 1), NULL));
}

TEST(EqOp, MatchesThroughNestedArray) {
    BSONObj operand = BSON("a.b.c.d" << 3);
    EqualityMatchExpression eq;
    eq.init("a.b.c.d", operand["a.b.c.d"]);
    BSONObj obj = fromjson("{a:{b:[{c:[{d:1},{d:2}]},{c:[{d:3}]}]}}");
    ASSERT(eq.matchesBSON(obj, NULL));
}

TEST(EqOp, ElemMatchKey) {
    BSONObj operand = BSON("a" << 5);
    EqualityMatchExpression eq;
    ASSERT(eq.init("a", operand["a"]).isOK());
    MatchDetails details;
    details.requestElemMatchKey();
    ASSERT(!eq.matchesBSON(BSON("a" << 4), &details));
    ASSERT(!details.hasElemMatchKey());
    ASSERT(eq.matchesBSON(BSON("a" << 5), &details));
    ASSERT(!details.hasElemMatchKey());
    ASSERT(eq.matchesBSON(BSON("a" << BSON_ARRAY(1 << 2 << 5)), &details));
    ASSERT(details.hasElemMatchKey());
    ASSERT_EQUALS("2", details.elemMatchKey());
}

// SERVER-14886: when an array is being traversed explictly at the same time that a nested array
// is being traversed implicitly, the elemMatch key should refer to the offset of the array
// being implicitly traversed.
TEST(EqOp, ElemMatchKeyWithImplicitAndExplicitTraversal) {
    BSONObj operand = BSON("a.0.b" << 3);
    BSONElement operandFirstElt = operand.firstElement();
    EqualityMatchExpression eq;
    ASSERT(eq.init(operandFirstElt.fieldName(), operandFirstElt).isOK());
    MatchDetails details;
    details.requestElemMatchKey();
    BSONObj obj = fromjson("{a: [{b: [2, 3]}, {b: [4, 5]}]}");
    ASSERT(eq.matchesBSON(obj, &details));
    ASSERT(details.hasElemMatchKey());
    ASSERT_EQUALS("1", details.elemMatchKey());
}

TEST(EqOp, Equality1) {
    EqualityMatchExpression eq1;
    EqualityMatchExpression eq2;
    EqualityMatchExpression eq3;

    BSONObj operand = BSON("a" << 5 << "b" << 5 << "c" << 4);

    eq1.init("a", operand["a"]);
    eq2.init("a", operand["b"]);
    eq3.init("c", operand["c"]);

    ASSERT(eq1.equivalent(&eq1));
    ASSERT(eq1.equivalent(&eq2));
    ASSERT(!eq1.equivalent(&eq3));
}

/**
   TEST( EqOp, MatchesIndexKeyScalar ) {
   BSONObj operand = BSON( "a" << 6 );
   EqualityMatchExpression eq;
   ASSERT( eq.init( "a", operand[ "a" ] ).isOK() );
   IndexSpec indexSpec( BSON( "a" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_True ==
   eq.matchesIndexKey( BSON( "" << 6 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_False ==
   eq.matchesIndexKey( BSON( "" << 4 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_False ==
   eq.matchesIndexKey( BSON( "" << BSON_ARRAY( 6 ) ), indexSpec ) );
   }

   TEST( EqOp, MatchesIndexKeyMissing ) {
   BSONObj operand = BSON( "a" << 6 );
   EqualityMatchExpression eq;
   ASSERT( eq.init( "a", operand[ "a" ] ).isOK() );
   IndexSpec indexSpec( BSON( "b" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   eq.matchesIndexKey( BSON( "" << 6 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   eq.matchesIndexKey( BSON( "" << 4 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   eq.matchesIndexKey( BSON( "" << BSON_ARRAY( 8 << 6 ) ), indexSpec ) );
   }

   TEST( EqOp, MatchesIndexKeyArray ) {
   BSONObj operand = BSON( "a" << BSON_ARRAY( 4 << 5 ) );
   ComparisonMatchExpression eq
   ASSERT( eq.init( "a", operand[ "a" ] ).isOK() );
   IndexSpec indexSpec( BSON( "a" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   eq.matchesIndexKey( BSON( "" << 4 ), indexSpec ) );
   }

   TEST( EqOp, MatchesIndexKeyArrayValue ) {
   BSONObj operand = BSON( "a" << 6 );
   ComparisonMatchExpression eq
   ASSERT( eq.init( "a", operand[ "a" ] ).isOK() );
   IndexSpec indexSpec( BSON( "loc" << "mockarrayvalue" << "a" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_True ==
   eq.matchesIndexKey( BSON( "" << "dummygeohash" << "" << 6 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_False ==
   eq.matchesIndexKey( BSON( "" << "dummygeohash" << "" << 4 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_True ==
   eq.matchesIndexKey( BSON( "" << "dummygeohash" <<
   "" << BSON_ARRAY( 8 << 6 ) ), indexSpec ) );
   }
*/
TEST(LtOp, MatchesElement) {
    BSONObj operand = BSON("$lt" << 5);
    BSONObj match = BSON("a" << 4.5);
    BSONObj notMatch = BSON("a" << 6);
    BSONObj notMatchEqual = BSON("a" << 5);
    BSONObj notMatchWrongType = BSON("a"
                                     << "foo");
    LTMatchExpression lt;
    ASSERT(lt.init("", operand["$lt"]).isOK());
    ASSERT(lt.matchesSingleElement(match.firstElement()));
    ASSERT(!lt.matchesSingleElement(notMatch.firstElement()));
    ASSERT(!lt.matchesSingleElement(notMatchEqual.firstElement()));
    ASSERT(!lt.matchesSingleElement(notMatchWrongType.firstElement()));
}

TEST(LtOp, InvalidEooOperand) {
    BSONObj operand;
    LTMatchExpression lt;
    ASSERT(!lt.init("", operand.firstElement()).isOK());
}

TEST(LtOp, MatchesScalar) {
    BSONObj operand = BSON("$lt" << 5);
    LTMatchExpression lt;
    ASSERT(lt.init("a", operand["$lt"]).isOK());
    ASSERT(lt.matchesBSON(BSON("a" << 4.5), NULL));
    ASSERT(!lt.matchesBSON(BSON("a" << 6), NULL));
}

TEST(LtOp, MatchesScalarEmptyKey) {
    BSONObj operand = BSON("$lt" << 5);
    LTMatchExpression lt;
    ASSERT(lt.init("", operand["$lt"]).isOK());
    ASSERT(lt.matchesBSON(BSON("" << 4.5), NULL));
    ASSERT(!lt.matchesBSON(BSON("" << 6), NULL));
}

TEST(LtOp, MatchesArrayValue) {
    BSONObj operand = BSON("$lt" << 5);
    LTMatchExpression lt;
    ASSERT(lt.init("a", operand["$lt"]).isOK());
    ASSERT(lt.matchesBSON(BSON("a" << BSON_ARRAY(6 << 4.5)), NULL));
    ASSERT(!lt.matchesBSON(BSON("a" << BSON_ARRAY(6 << 7)), NULL));
}

TEST(LtOp, MatchesWholeArray) {
    BSONObj operand = BSON("$lt" << BSON_ARRAY(5));
    LTMatchExpression lt;
    ASSERT(lt.init("a", operand["$lt"]).isOK());
    ASSERT(lt.matchesBSON(BSON("a" << BSON_ARRAY(4)), NULL));
    ASSERT(!lt.matchesBSON(BSON("a" << BSON_ARRAY(5)), NULL));
    ASSERT(!lt.matchesBSON(BSON("a" << BSON_ARRAY(6)), NULL));
    // Nested array.
    ASSERT(lt.matchesBSON(BSON("a" << BSON_ARRAY(BSON_ARRAY(4))), NULL));
    ASSERT(!lt.matchesBSON(BSON("a" << BSON_ARRAY(BSON_ARRAY(5))), NULL));
    ASSERT(!lt.matchesBSON(BSON("a" << BSON_ARRAY(BSON_ARRAY(6))), NULL));
}

TEST(LtOp, MatchesNull) {
    BSONObj operand = BSON("$lt" << BSONNULL);
    LTMatchExpression lt;
    ASSERT(lt.init("a", operand["$lt"]).isOK());
    ASSERT(!lt.matchesBSON(BSONObj(), NULL));
    ASSERT(!lt.matchesBSON(BSON("a" << BSONNULL), NULL));
    ASSERT(!lt.matchesBSON(BSON("a" << 4), NULL));
    // A non-existent field is treated same way as an empty bson object
    ASSERT(!lt.matchesBSON(BSON("b" << 4), NULL));
}

TEST(LtOp, MatchesDotNotationNull) {
    BSONObj operand = BSON("$lt" << BSONNULL);
    LTMatchExpression lt;
    ASSERT(lt.init("a.b", operand["$lt"]).isOK());
    ASSERT(!lt.matchesBSON(BSONObj(), NULL));
    ASSERT(!lt.matchesBSON(BSON("a" << BSONNULL), NULL));
    ASSERT(!lt.matchesBSON(BSON("a" << 4), NULL));
    ASSERT(!lt.matchesBSON(BSON("a" << BSONObj()), NULL));
    ASSERT(!lt.matchesBSON(BSON("a" << BSON_ARRAY(BSON("b" << BSONNULL))), NULL));
    ASSERT(!lt.matchesBSON(BSON("a" << BSON_ARRAY(BSON("a" << 4) << BSON("b" << 4))), NULL));
    ASSERT(!lt.matchesBSON(BSON("a" << BSON_ARRAY(4)), NULL));
    ASSERT(!lt.matchesBSON(BSON("a" << BSON_ARRAY(BSON("b" << 4))), NULL));
}

TEST(LtOp, MatchesMinKey) {
    BSONObj operand = BSON("a" << MinKey);
    LTMatchExpression lt;
    ASSERT(lt.init("a", operand["a"]).isOK());
    ASSERT(!lt.matchesBSON(BSON("a" << MinKey), NULL));
    ASSERT(!lt.matchesBSON(BSON("a" << MaxKey), NULL));
    ASSERT(!lt.matchesBSON(BSON("a" << 4), NULL));
}

TEST(LtOp, MatchesMaxKey) {
    BSONObj operand = BSON("a" << MaxKey);
    LTMatchExpression lt;
    ASSERT(lt.init("a", operand["a"]).isOK());
    ASSERT(!lt.matchesBSON(BSON("a" << MaxKey), NULL));
    ASSERT(lt.matchesBSON(BSON("a" << MinKey), NULL));
    ASSERT(lt.matchesBSON(BSON("a" << 4), NULL));
}

TEST(LtOp, ElemMatchKey) {
    BSONObj operand = BSON("$lt" << 5);
    LTMatchExpression lt;
    ASSERT(lt.init("a", operand["$lt"]).isOK());
    MatchDetails details;
    details.requestElemMatchKey();
    ASSERT(!lt.matchesBSON(BSON("a" << 6), &details));
    ASSERT(!details.hasElemMatchKey());
    ASSERT(lt.matchesBSON(BSON("a" << 4), &details));
    ASSERT(!details.hasElemMatchKey());
    ASSERT(lt.matchesBSON(BSON("a" << BSON_ARRAY(6 << 2 << 5)), &details));
    ASSERT(details.hasElemMatchKey());
    ASSERT_EQUALS("1", details.elemMatchKey());
}

/**
   TEST( LtOp, MatchesIndexKeyScalar ) {
   BSONObj operand = BSON( "$lt" << 6 );
   LtOp lt;
   ASSERT( lt.init( "a", operand[ "$lt" ] ).isOK() );
   IndexSpec indexSpec( BSON( "a" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_True ==
   lt.matchesIndexKey( BSON( "" << 3 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_False ==
   lt.matchesIndexKey( BSON( "" << 6 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_False ==
   lt.matchesIndexKey( BSON( "" << BSON_ARRAY( 5 ) ), indexSpec ) );
   }

   TEST( LtOp, MatchesIndexKeyMissing ) {
   BSONObj operand = BSON( "$lt" << 6 );
   LtOp lt;
   ASSERT( lt.init( "a", operand[ "$lt" ] ).isOK() );
   IndexSpec indexSpec( BSON( "b" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   lt.matchesIndexKey( BSON( "" << 6 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   lt.matchesIndexKey( BSON( "" << 4 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   lt.matchesIndexKey( BSON( "" << BSON_ARRAY( 8 << 6 ) ), indexSpec ) );
   }

   TEST( LtOp, MatchesIndexKeyArray ) {
   BSONObj operand = BSON( "$lt" << BSON_ARRAY( 4 << 5 ) );
   LtOp lt;
   ASSERT( lt.init( "a", operand[ "$lt" ] ).isOK() );
   IndexSpec indexSpec( BSON( "a" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   lt.matchesIndexKey( BSON( "" << 3 ), indexSpec ) );
   }

   TEST( LtOp, MatchesIndexKeyArrayValue ) {
   BSONObj operand = BSON( "$lt" << 6 );
   LtOp lt;
   ASSERT( lt.init( "a", operand[ "$lt" ] ).isOK() );
   IndexSpec indexSpec( BSON( "loc" << "mockarrayvalue" << "a" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_True ==
   lt.matchesIndexKey( BSON( "" << "dummygeohash" << "" << 3 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_False ==
   lt.matchesIndexKey( BSON( "" << "dummygeohash" << "" << 6 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_True ==
   lt.matchesIndexKey( BSON( "" << "dummygeohash" <<
   "" << BSON_ARRAY( 8 << 6 << 4 ) ), indexSpec ) );
   }
*/
TEST(LteOp, MatchesElement) {
    BSONObj operand = BSON("$lte" << 5);
    BSONObj match = BSON("a" << 4.5);
    BSONObj equalMatch = BSON("a" << 5);
    BSONObj notMatch = BSON("a" << 6);
    BSONObj notMatchWrongType = BSON("a"
                                     << "foo");
    LTEMatchExpression lte;
    ASSERT(lte.init("", operand["$lte"]).isOK());
    ASSERT(lte.matchesSingleElement(match.firstElement()));
    ASSERT(lte.matchesSingleElement(equalMatch.firstElement()));
    ASSERT(!lte.matchesSingleElement(notMatch.firstElement()));
    ASSERT(!lte.matchesSingleElement(notMatchWrongType.firstElement()));
}

TEST(LteOp, InvalidEooOperand) {
    BSONObj operand;
    LTEMatchExpression lte;
    ASSERT(!lte.init("", operand.firstElement()).isOK());
}

TEST(LteOp, MatchesScalar) {
    BSONObj operand = BSON("$lte" << 5);
    LTEMatchExpression lte;
    ASSERT(lte.init("a", operand["$lte"]).isOK());
    ASSERT(lte.matchesBSON(BSON("a" << 4.5), NULL));
    ASSERT(!lte.matchesBSON(BSON("a" << 6), NULL));
}

TEST(LteOp, MatchesArrayValue) {
    BSONObj operand = BSON("$lte" << 5);
    LTEMatchExpression lte;
    ASSERT(lte.init("a", operand["$lte"]).isOK());
    ASSERT(lte.matchesBSON(BSON("a" << BSON_ARRAY(6 << 4.5)), NULL));
    ASSERT(!lte.matchesBSON(BSON("a" << BSON_ARRAY(6 << 7)), NULL));
}

TEST(LteOp, MatchesWholeArray) {
    BSONObj operand = BSON("$lte" << BSON_ARRAY(5));
    LTEMatchExpression lte;
    ASSERT(lte.init("a", operand["$lte"]).isOK());
    ASSERT(lte.matchesBSON(BSON("a" << BSON_ARRAY(4)), NULL));
    ASSERT(lte.matchesBSON(BSON("a" << BSON_ARRAY(5)), NULL));
    ASSERT(!lte.matchesBSON(BSON("a" << BSON_ARRAY(6)), NULL));
    // Nested array.
    ASSERT(lte.matchesBSON(BSON("a" << BSON_ARRAY(BSON_ARRAY(4))), NULL));
    ASSERT(lte.matchesBSON(BSON("a" << BSON_ARRAY(BSON_ARRAY(5))), NULL));
    ASSERT(!lte.matchesBSON(BSON("a" << BSON_ARRAY(BSON_ARRAY(6))), NULL));
}

TEST(LteOp, MatchesNull) {
    BSONObj operand = BSON("$lte" << BSONNULL);
    LTEMatchExpression lte;
    ASSERT(lte.init("a", operand["$lte"]).isOK());
    ASSERT(lte.matchesBSON(BSONObj(), NULL));
    ASSERT(lte.matchesBSON(BSON("a" << BSONNULL), NULL));
    ASSERT(!lte.matchesBSON(BSON("a" << 4), NULL));
    // A non-existent field is treated same way as an empty bson object
    ASSERT(lte.matchesBSON(BSON("b" << 4), NULL));
}

TEST(LteOp, MatchesDotNotationNull) {
    BSONObj operand = BSON("$lte" << BSONNULL);
    LTEMatchExpression lte;
    ASSERT(lte.init("a.b", operand["$lte"]).isOK());
    ASSERT(lte.matchesBSON(BSONObj(), NULL));
    ASSERT(lte.matchesBSON(BSON("a" << BSONNULL), NULL));
    ASSERT(lte.matchesBSON(BSON("a" << 4), NULL));
    ASSERT(lte.matchesBSON(BSON("a" << BSONObj()), NULL));
    ASSERT(lte.matchesBSON(BSON("a" << BSON_ARRAY(BSON("b" << BSONNULL))), NULL));
    ASSERT(lte.matchesBSON(BSON("a" << BSON_ARRAY(BSON("a" << 4) << BSON("b" << 4))), NULL));
    ASSERT(!lte.matchesBSON(BSON("a" << BSON_ARRAY(4)), NULL));
    ASSERT(!lte.matchesBSON(BSON("a" << BSON_ARRAY(BSON("b" << 4))), NULL));
}

TEST(LteOp, MatchesMinKey) {
    BSONObj operand = BSON("a" << MinKey);
    LTEMatchExpression lte;
    ASSERT(lte.init("a", operand["a"]).isOK());
    ASSERT(lte.matchesBSON(BSON("a" << MinKey), NULL));
    ASSERT(!lte.matchesBSON(BSON("a" << MaxKey), NULL));
    ASSERT(!lte.matchesBSON(BSON("a" << 4), NULL));
}

TEST(LteOp, MatchesMaxKey) {
    BSONObj operand = BSON("a" << MaxKey);
    LTEMatchExpression lte;
    ASSERT(lte.init("a", operand["a"]).isOK());
    ASSERT(lte.matchesBSON(BSON("a" << MaxKey), NULL));
    ASSERT(lte.matchesBSON(BSON("a" << MinKey), NULL));
    ASSERT(lte.matchesBSON(BSON("a" << 4), NULL));
}


TEST(LteOp, ElemMatchKey) {
    BSONObj operand = BSON("$lte" << 5);
    LTEMatchExpression lte;
    ASSERT(lte.init("a", operand["$lte"]).isOK());
    MatchDetails details;
    details.requestElemMatchKey();
    ASSERT(!lte.matchesBSON(BSON("a" << 6), &details));
    ASSERT(!details.hasElemMatchKey());
    ASSERT(lte.matchesBSON(BSON("a" << 4), &details));
    ASSERT(!details.hasElemMatchKey());
    ASSERT(lte.matchesBSON(BSON("a" << BSON_ARRAY(6 << 2 << 5)), &details));
    ASSERT(details.hasElemMatchKey());
    ASSERT_EQUALS("1", details.elemMatchKey());
}

/**
   TEST( LteOp, MatchesIndexKeyScalar ) {
   BSONObj operand = BSON( "$lte" << 6 );
   LteOp lte;
   ASSERT( lte.init( "a", operand[ "$lte" ] ).isOK() );
   IndexSpec indexSpec( BSON( "a" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_True ==
   lte.matchesIndexKey( BSON( "" << 6 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_False ==
   lte.matchesIndexKey( BSON( "" << 7 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_False ==
   lte.matchesIndexKey( BSON( "" << BSON_ARRAY( 5 ) ), indexSpec ) );
   }

   TEST( LteOp, MatchesIndexKeyMissing ) {
   BSONObj operand = BSON( "$lte" << 6 );
   LteOp lte;
   ASSERT( lte.init( "a", operand[ "$lte" ] ).isOK() );
   IndexSpec indexSpec( BSON( "b" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   lte.matchesIndexKey( BSON( "" << 7 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   lte.matchesIndexKey( BSON( "" << 4 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   lte.matchesIndexKey( BSON( "" << BSON_ARRAY( 8 << 6 ) ), indexSpec ) );
   }

   TEST( LteOp, MatchesIndexKeyArray ) {
   BSONObj operand = BSON( "$lte" << BSON_ARRAY( 4 << 5 ) );
   LteOp lte;
   ASSERT( lte.init( "a", operand[ "$lte" ] ).isOK() );
   IndexSpec indexSpec( BSON( "a" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   lte.matchesIndexKey( BSON( "" << 3 ), indexSpec ) );
   }

   TEST( LteOp, MatchesIndexKeyArrayValue ) {
   BSONObj operand = BSON( "$lte" << 6 );
   LteOp lte;
   ASSERT( lte.init( "a", operand[ "$lte" ] ).isOK() );
   IndexSpec indexSpec( BSON( "loc" << "mockarrayvalue" << "a" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_True ==
   lte.matchesIndexKey( BSON( "" << "dummygeohash" << "" << 3 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_False ==
   lte.matchesIndexKey( BSON( "" << "dummygeohash" << "" << 7 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_True ==
   lte.matchesIndexKey( BSON( "" << "dummygeohash" <<
   "" << BSON_ARRAY( 8 << 6 << 4 ) ), indexSpec ) );
   }

   TEST( GtOp, MatchesElement ) {
   BSONObj operand = BSON( "$gt" << 5 );
   BSONObj match = BSON( "a" << 5.5 );
   BSONObj notMatch = BSON( "a" << 4 );
   BSONObj notMatchEqual = BSON( "a" << 5 );
   BSONObj notMatchWrongType = BSON( "a" << "foo" );
   GtOp gt;
   ASSERT( gt.init( "", operand[ "$gt" ] ).isOK() );
   ASSERT( gt.matchesSingleElement( match.firstElement() ) );
   ASSERT( !gt.matchesSingleElement( notMatch.firstElement() ) );
   ASSERT( !gt.matchesSingleElement( notMatchEqual.firstElement() ) );
   ASSERT( !gt.matchesSingleElement( notMatchWrongType.firstElement() ) );
   }
*/

TEST(GtOp, InvalidEooOperand) {
    BSONObj operand;
    GTMatchExpression gt;
    ASSERT(!gt.init("", operand.firstElement()).isOK());
}

TEST(GtOp, MatchesScalar) {
    BSONObj operand = BSON("$gt" << 5);
    GTMatchExpression gt;
    ASSERT(gt.init("a", operand["$gt"]).isOK());
    ASSERT(gt.matchesBSON(BSON("a" << 5.5), NULL));
    ASSERT(!gt.matchesBSON(BSON("a" << 4), NULL));
}

TEST(GtOp, MatchesArrayValue) {
    BSONObj operand = BSON("$gt" << 5);
    GTMatchExpression gt;
    ASSERT(gt.init("a", operand["$gt"]).isOK());
    ASSERT(gt.matchesBSON(BSON("a" << BSON_ARRAY(3 << 5.5)), NULL));
    ASSERT(!gt.matchesBSON(BSON("a" << BSON_ARRAY(2 << 4)), NULL));
}

TEST(GtOp, MatchesWholeArray) {
    BSONObj operand = BSON("$gt" << BSON_ARRAY(5));
    GTMatchExpression gt;
    ASSERT(gt.init("a", operand["$gt"]).isOK());
    ASSERT(!gt.matchesBSON(BSON("a" << BSON_ARRAY(4)), NULL));
    ASSERT(!gt.matchesBSON(BSON("a" << BSON_ARRAY(5)), NULL));
    ASSERT(gt.matchesBSON(BSON("a" << BSON_ARRAY(6)), NULL));
    // Nested array.
    // XXX: The following assertion documents current behavior.
    ASSERT(gt.matchesBSON(BSON("a" << BSON_ARRAY(BSON_ARRAY(4))), NULL));
    // XXX: The following assertion documents current behavior.
    ASSERT(gt.matchesBSON(BSON("a" << BSON_ARRAY(BSON_ARRAY(5))), NULL));
    ASSERT(gt.matchesBSON(BSON("a" << BSON_ARRAY(BSON_ARRAY(6))), NULL));
}

TEST(GtOp, MatchesNull) {
    BSONObj operand = BSON("$gt" << BSONNULL);
    GTMatchExpression gt;
    ASSERT(gt.init("a", operand["$gt"]).isOK());
    ASSERT(!gt.matchesBSON(BSONObj(), NULL));
    ASSERT(!gt.matchesBSON(BSON("a" << BSONNULL), NULL));
    ASSERT(!gt.matchesBSON(BSON("a" << 4), NULL));
    // A non-existent field is treated same way as an empty bson object
    ASSERT(!gt.matchesBSON(BSON("b" << 4), NULL));
}

TEST(GtOp, MatchesDotNotationNull) {
    BSONObj operand = BSON("$gt" << BSONNULL);
    GTMatchExpression gt;
    ASSERT(gt.init("a.b", operand["$gt"]).isOK());
    ASSERT(!gt.matchesBSON(BSONObj(), NULL));
    ASSERT(!gt.matchesBSON(BSON("a" << BSONNULL), NULL));
    ASSERT(!gt.matchesBSON(BSON("a" << 4), NULL));
    ASSERT(!gt.matchesBSON(BSON("a" << BSONObj()), NULL));
    ASSERT(!gt.matchesBSON(BSON("a" << BSON_ARRAY(BSON("b" << BSONNULL))), NULL));
    ASSERT(!gt.matchesBSON(BSON("a" << BSON_ARRAY(BSON("a" << 4) << BSON("b" << 4))), NULL));
    ASSERT(!gt.matchesBSON(BSON("a" << BSON_ARRAY(4)), NULL));
    ASSERT(!gt.matchesBSON(BSON("a" << BSON_ARRAY(BSON("b" << 4))), NULL));
}

TEST(GtOp, MatchesMinKey) {
    BSONObj operand = BSON("a" << MinKey);
    GTMatchExpression gt;
    ASSERT(gt.init("a", operand["a"]).isOK());
    ASSERT(!gt.matchesBSON(BSON("a" << MinKey), NULL));
    ASSERT(gt.matchesBSON(BSON("a" << MaxKey), NULL));
    ASSERT(gt.matchesBSON(BSON("a" << 4), NULL));
}

TEST(GtOp, MatchesMaxKey) {
    BSONObj operand = BSON("a" << MaxKey);
    GTMatchExpression gt;
    ASSERT(gt.init("a", operand["a"]).isOK());
    ASSERT(!gt.matchesBSON(BSON("a" << MaxKey), NULL));
    ASSERT(!gt.matchesBSON(BSON("a" << MinKey), NULL));
    ASSERT(!gt.matchesBSON(BSON("a" << 4), NULL));
}

TEST(GtOp, ElemMatchKey) {
    BSONObj operand = BSON("$gt" << 5);
    GTMatchExpression gt;
    ASSERT(gt.init("a", operand["$gt"]).isOK());
    MatchDetails details;
    details.requestElemMatchKey();
    ASSERT(!gt.matchesBSON(BSON("a" << 4), &details));
    ASSERT(!details.hasElemMatchKey());
    ASSERT(gt.matchesBSON(BSON("a" << 6), &details));
    ASSERT(!details.hasElemMatchKey());
    ASSERT(gt.matchesBSON(BSON("a" << BSON_ARRAY(2 << 6 << 5)), &details));
    ASSERT(details.hasElemMatchKey());
    ASSERT_EQUALS("1", details.elemMatchKey());
}

/**
   TEST( GtOp, MatchesIndexKeyScalar ) {
   BSONObj operand = BSON( "$gt" << 6 );
   GtOp gt;
   ASSERT( gt.init( "a", operand[ "$gt" ] ).isOK() );
   IndexSpec indexSpec( BSON( "a" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_True ==
   gt.matchesIndexKey( BSON( "" << 7 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_False ==
   gt.matchesIndexKey( BSON( "" << 6 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_False ==
   gt.matchesIndexKey( BSON( "" << BSON_ARRAY( 9 ) ), indexSpec ) );
   }

   TEST( GtOp, MatchesIndexKeyMissing ) {
   BSONObj operand = BSON( "$gt" << 6 );
   GtOp gt;
   ASSERT( gt.init( "a", operand[ "$gt" ] ).isOK() );
   IndexSpec indexSpec( BSON( "b" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   gt.matchesIndexKey( BSON( "" << 7 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   gt.matchesIndexKey( BSON( "" << 4 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   gt.matchesIndexKey( BSON( "" << BSON_ARRAY( 8 << 6 ) ), indexSpec ) );
   }

   TEST( GtOp, MatchesIndexKeyArray ) {
   BSONObj operand = BSON( "$gt" << BSON_ARRAY( 4 << 5 ) );
   GtOp gt;
   ASSERT( gt.init( "a", operand[ "$gt" ] ).isOK() );
   IndexSpec indexSpec( BSON( "a" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   gt.matchesIndexKey( BSON( "" << 8 ), indexSpec ) );
   }

   TEST( GtOp, MatchesIndexKeyArrayValue ) {
   BSONObj operand = BSON( "$gt" << 6 );
   GtOp gt;
   ASSERT( gt.init( "a", operand[ "$gt" ] ).isOK() );
   IndexSpec indexSpec( BSON( "loc" << "mockarrayvalue" << "a" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_True ==
   gt.matchesIndexKey( BSON( "" << "dummygeohash" << "" << 7 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_False ==
   gt.matchesIndexKey( BSON( "" << "dummygeohash" << "" << 3 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_True ==
   gt.matchesIndexKey( BSON( "" << "dummygeohash" <<
   "" << BSON_ARRAY( 8 << 6 << 4 ) ), indexSpec ) );
   }
*/

TEST(ComparisonMatchExpression, MatchesElement) {
    BSONObj operand = BSON("$gte" << 5);
    BSONObj match = BSON("a" << 5.5);
    BSONObj equalMatch = BSON("a" << 5);
    BSONObj notMatch = BSON("a" << 4);
    BSONObj notMatchWrongType = BSON("a"
                                     << "foo");
    GTEMatchExpression gte;
    ASSERT(gte.init("", operand["$gte"]).isOK());
    ASSERT(gte.matchesSingleElement(match.firstElement()));
    ASSERT(gte.matchesSingleElement(equalMatch.firstElement()));
    ASSERT(!gte.matchesSingleElement(notMatch.firstElement()));
    ASSERT(!gte.matchesSingleElement(notMatchWrongType.firstElement()));
}

TEST(ComparisonMatchExpression, InvalidEooOperand) {
    BSONObj operand;
    GTEMatchExpression gte;
    ASSERT(!gte.init("", operand.firstElement()).isOK());
}

TEST(ComparisonMatchExpression, MatchesScalar) {
    BSONObj operand = BSON("$gte" << 5);
    GTEMatchExpression gte;
    ASSERT(gte.init("a", operand["$gte"]).isOK());
    ASSERT(gte.matchesBSON(BSON("a" << 5.5), NULL));
    ASSERT(!gte.matchesBSON(BSON("a" << 4), NULL));
}

TEST(ComparisonMatchExpression, MatchesArrayValue) {
    BSONObj operand = BSON("$gte" << 5);
    GTEMatchExpression gte;
    ASSERT(gte.init("a", operand["$gte"]).isOK());
    ASSERT(gte.matchesBSON(BSON("a" << BSON_ARRAY(4 << 5.5)), NULL));
    ASSERT(!gte.matchesBSON(BSON("a" << BSON_ARRAY(1 << 2)), NULL));
}

TEST(ComparisonMatchExpression, MatchesWholeArray) {
    BSONObj operand = BSON("$gte" << BSON_ARRAY(5));
    GTEMatchExpression gte;
    ASSERT(gte.init("a", operand["$gte"]).isOK());
    ASSERT(!gte.matchesBSON(BSON("a" << BSON_ARRAY(4)), NULL));
    ASSERT(gte.matchesBSON(BSON("a" << BSON_ARRAY(5)), NULL));
    ASSERT(gte.matchesBSON(BSON("a" << BSON_ARRAY(6)), NULL));
    // Nested array.
    // XXX: The following assertion documents current behavior.
    ASSERT(gte.matchesBSON(BSON("a" << BSON_ARRAY(BSON_ARRAY(4))), NULL));
    ASSERT(gte.matchesBSON(BSON("a" << BSON_ARRAY(BSON_ARRAY(5))), NULL));
    ASSERT(gte.matchesBSON(BSON("a" << BSON_ARRAY(BSON_ARRAY(6))), NULL));
}

TEST(ComparisonMatchExpression, MatchesNull) {
    BSONObj operand = BSON("$gte" << BSONNULL);
    GTEMatchExpression gte;
    ASSERT(gte.init("a", operand["$gte"]).isOK());
    ASSERT(gte.matchesBSON(BSONObj(), NULL));
    ASSERT(gte.matchesBSON(BSON("a" << BSONNULL), NULL));
    ASSERT(!gte.matchesBSON(BSON("a" << 4), NULL));
    // A non-existent field is treated same way as an empty bson object
    ASSERT(gte.matchesBSON(BSON("b" << 4), NULL));
}

TEST(ComparisonMatchExpression, MatchesDotNotationNull) {
    BSONObj operand = BSON("$gte" << BSONNULL);
    GTEMatchExpression gte;
    ASSERT(gte.init("a.b", operand["$gte"]).isOK());
    ASSERT(gte.matchesBSON(BSONObj(), NULL));
    ASSERT(gte.matchesBSON(BSON("a" << BSONNULL), NULL));
    ASSERT(gte.matchesBSON(BSON("a" << 4), NULL));
    ASSERT(gte.matchesBSON(BSON("a" << BSONObj()), NULL));
    ASSERT(gte.matchesBSON(BSON("a" << BSON_ARRAY(BSON("b" << BSONNULL))), NULL));
    ASSERT(gte.matchesBSON(BSON("a" << BSON_ARRAY(BSON("a" << 4) << BSON("b" << 4))), NULL));
    ASSERT(!gte.matchesBSON(BSON("a" << BSON_ARRAY(4)), NULL));
    ASSERT(!gte.matchesBSON(BSON("a" << BSON_ARRAY(BSON("b" << 4))), NULL));
}

TEST(ComparisonMatchExpression, MatchesMinKey) {
    BSONObj operand = BSON("a" << MinKey);
    GTEMatchExpression gte;
    ASSERT(gte.init("a", operand["a"]).isOK());
    ASSERT(gte.matchesBSON(BSON("a" << MinKey), NULL));
    ASSERT(gte.matchesBSON(BSON("a" << MaxKey), NULL));
    ASSERT(gte.matchesBSON(BSON("a" << 4), NULL));
}

TEST(ComparisonMatchExpression, MatchesMaxKey) {
    BSONObj operand = BSON("a" << MaxKey);
    GTEMatchExpression gte;
    ASSERT(gte.init("a", operand["a"]).isOK());
    ASSERT(gte.matchesBSON(BSON("a" << MaxKey), NULL));
    ASSERT(!gte.matchesBSON(BSON("a" << MinKey), NULL));
    ASSERT(!gte.matchesBSON(BSON("a" << 4), NULL));
}

TEST(ComparisonMatchExpression, ElemMatchKey) {
    BSONObj operand = BSON("$gte" << 5);
    GTEMatchExpression gte;
    ASSERT(gte.init("a", operand["$gte"]).isOK());
    MatchDetails details;
    details.requestElemMatchKey();
    ASSERT(!gte.matchesBSON(BSON("a" << 4), &details));
    ASSERT(!details.hasElemMatchKey());
    ASSERT(gte.matchesBSON(BSON("a" << 6), &details));
    ASSERT(!details.hasElemMatchKey());
    ASSERT(gte.matchesBSON(BSON("a" << BSON_ARRAY(2 << 6 << 5)), &details));
    ASSERT(details.hasElemMatchKey());
    ASSERT_EQUALS("1", details.elemMatchKey());
}

/**
   TEST( GteOp, MatchesIndexKeyScalar ) {
   BSONObj operand = BSON( "$gte" << 6 );
   GteOp gte;
   ASSERT( gte.init( "a", operand[ "$gte" ] ).isOK() );
   IndexSpec indexSpec( BSON( "a" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_True ==
   gte.matchesIndexKey( BSON( "" << 6 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_False ==
   gte.matchesIndexKey( BSON( "" << 5 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_False ==
   gte.matchesIndexKey( BSON( "" << BSON_ARRAY( 7 ) ), indexSpec ) );
   }

   TEST( GteOp, MatchesIndexKeyMissing ) {
   BSONObj operand = BSON( "$gte" << 6 );
   GteOp gte;
   ASSERT( gte.init( "a", operand[ "$gte" ] ).isOK() );
   IndexSpec indexSpec( BSON( "b" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   gte.matchesIndexKey( BSON( "" << 6 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   gte.matchesIndexKey( BSON( "" << 4 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   gte.matchesIndexKey( BSON( "" << BSON_ARRAY( 8 << 6 ) ), indexSpec ) );
   }

   TEST( GteOp, MatchesIndexKeyArray ) {
   BSONObj operand = BSON( "$gte" << BSON_ARRAY( 4 << 5 ) );
   GteOp gte;
   ASSERT( gte.init( "a", operand[ "$gte" ] ).isOK() );
   IndexSpec indexSpec( BSON( "a" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   gte.matchesIndexKey( BSON( "" << 6 ), indexSpec ) );
   }

   TEST( GteOp, MatchesIndexKeyArrayValue ) {
   BSONObj operand = BSON( "$gte" << 6 );
   GteOp gte;
   ASSERT( gte.init( "a", operand[ "$gte" ] ).isOK() );
   IndexSpec indexSpec( BSON( "loc" << "mockarrayvalue" << "a" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_True ==
   gte.matchesIndexKey( BSON( "" << "dummygeohash" << "" << 6 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_False ==
   gte.matchesIndexKey( BSON( "" << "dummygeohash" << "" << 3 ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_True ==
   gte.matchesIndexKey( BSON( "" << "dummygeohash" <<
   "" << BSON_ARRAY( 8 << 6 << 4 ) ), indexSpec ) );
   }
*/

TEST(RegexMatchExpression, MatchesElementExact) {
    BSONObj match = BSON("a"
                         << "b");
    BSONObj notMatch = BSON("a"
                            << "c");
    RegexMatchExpression regex;
    ASSERT(regex.init("", "b", "").isOK());
    ASSERT(regex.matchesSingleElement(match.firstElement()));
    ASSERT(!regex.matchesSingleElement(notMatch.firstElement()));
}

TEST(RegexMatchExpression, TooLargePattern) {
    string tooLargePattern(50 * 1000, 'z');
    RegexMatchExpression regex;
    ASSERT(!regex.init("a", tooLargePattern, "").isOK());
}

TEST(RegexMatchExpression, MatchesElementSimplePrefix) {
    BSONObj match = BSON("x"
                         << "abc");
    BSONObj notMatch = BSON("x"
                            << "adz");
    RegexMatchExpression regex;
    ASSERT(regex.init("", "^ab", "").isOK());
    ASSERT(regex.matchesSingleElement(match.firstElement()));
    ASSERT(!regex.matchesSingleElement(notMatch.firstElement()));
}

TEST(RegexMatchExpression, MatchesElementCaseSensitive) {
    BSONObj match = BSON("x"
                         << "abc");
    BSONObj notMatch = BSON("x"
                            << "ABC");
    RegexMatchExpression regex;
    ASSERT(regex.init("", "abc", "").isOK());
    ASSERT(regex.matchesSingleElement(match.firstElement()));
    ASSERT(!regex.matchesSingleElement(notMatch.firstElement()));
}

TEST(RegexMatchExpression, MatchesElementCaseInsensitive) {
    BSONObj match = BSON("x"
                         << "abc");
    BSONObj matchUppercase = BSON("x"
                                  << "ABC");
    BSONObj notMatch = BSON("x"
                            << "abz");
    RegexMatchExpression regex;
    ASSERT(regex.init("", "abc", "i").isOK());
    ASSERT(regex.matchesSingleElement(match.firstElement()));
    ASSERT(regex.matchesSingleElement(matchUppercase.firstElement()));
    ASSERT(!regex.matchesSingleElement(notMatch.firstElement()));
}

TEST(RegexMatchExpression, MatchesElementMultilineOff) {
    BSONObj match = BSON("x"
                         << "az");
    BSONObj notMatch = BSON("x"
                            << "\naz");
    RegexMatchExpression regex;
    ASSERT(regex.init("", "^a", "").isOK());
    ASSERT(regex.matchesSingleElement(match.firstElement()));
    ASSERT(!regex.matchesSingleElement(notMatch.firstElement()));
}

TEST(RegexMatchExpression, MatchesElementMultilineOn) {
    BSONObj match = BSON("x"
                         << "az");
    BSONObj matchMultiline = BSON("x"
                                  << "\naz");
    BSONObj notMatch = BSON("x"
                            << "\n\n");
    RegexMatchExpression regex;
    ASSERT(regex.init("", "^a", "m").isOK());
    ASSERT(regex.matchesSingleElement(match.firstElement()));
    ASSERT(regex.matchesSingleElement(matchMultiline.firstElement()));
    ASSERT(!regex.matchesSingleElement(notMatch.firstElement()));
}

TEST(RegexMatchExpression, MatchesElementExtendedOff) {
    BSONObj match = BSON("x"
                         << "a b");
    BSONObj notMatch = BSON("x"
                            << "ab");
    RegexMatchExpression regex;
    ASSERT(regex.init("", "a b", "").isOK());
    ASSERT(regex.matchesSingleElement(match.firstElement()));
    ASSERT(!regex.matchesSingleElement(notMatch.firstElement()));
}

TEST(RegexMatchExpression, MatchesElementExtendedOn) {
    BSONObj match = BSON("x"
                         << "ab");
    BSONObj notMatch = BSON("x"
                            << "a b");
    RegexMatchExpression regex;
    ASSERT(regex.init("", "a b", "x").isOK());
    ASSERT(regex.matchesSingleElement(match.firstElement()));
    ASSERT(!regex.matchesSingleElement(notMatch.firstElement()));
}

TEST(RegexMatchExpression, MatchesElementDotAllOff) {
    BSONObj match = BSON("x"
                         << "a b");
    BSONObj notMatch = BSON("x"
                            << "a\nb");
    RegexMatchExpression regex;
    ASSERT(regex.init("", "a.b", "").isOK());
    ASSERT(regex.matchesSingleElement(match.firstElement()));
    ASSERT(!regex.matchesSingleElement(notMatch.firstElement()));
}

TEST(RegexMatchExpression, MatchesElementDotAllOn) {
    BSONObj match = BSON("x"
                         << "a b");
    BSONObj matchDotAll = BSON("x"
                               << "a\nb");
    BSONObj notMatch = BSON("x"
                            << "ab");
    RegexMatchExpression regex;
    ASSERT(regex.init("", "a.b", "s").isOK());
    ASSERT(regex.matchesSingleElement(match.firstElement()));
    ASSERT(regex.matchesSingleElement(matchDotAll.firstElement()));
    ASSERT(!regex.matchesSingleElement(notMatch.firstElement()));
}

TEST(RegexMatchExpression, MatchesElementMultipleFlags) {
    BSONObj matchMultilineDotAll = BSON("x"
                                        << "\na\nb");
    RegexMatchExpression regex;
    ASSERT(regex.init("", "^a.b", "ms").isOK());
    ASSERT(regex.matchesSingleElement(matchMultilineDotAll.firstElement()));
}

TEST(RegexMatchExpression, MatchesElementRegexType) {
    BSONObj match = BSONObjBuilder().appendRegex("x", "yz", "i").obj();
    BSONObj notMatchPattern = BSONObjBuilder().appendRegex("x", "r", "i").obj();
    BSONObj notMatchFlags = BSONObjBuilder().appendRegex("x", "yz", "s").obj();
    RegexMatchExpression regex;
    ASSERT(regex.init("", "yz", "i").isOK());
    ASSERT(regex.matchesSingleElement(match.firstElement()));
    ASSERT(!regex.matchesSingleElement(notMatchPattern.firstElement()));
    ASSERT(!regex.matchesSingleElement(notMatchFlags.firstElement()));
}

TEST(RegexMatchExpression, MatchesElementSymbolType) {
    BSONObj match = BSONObjBuilder().appendSymbol("x", "yz").obj();
    BSONObj notMatch = BSONObjBuilder().appendSymbol("x", "gg").obj();
    RegexMatchExpression regex;
    ASSERT(regex.init("", "yz", "").isOK());
    ASSERT(regex.matchesSingleElement(match.firstElement()));
    ASSERT(!regex.matchesSingleElement(notMatch.firstElement()));
}

TEST(RegexMatchExpression, MatchesElementWrongType) {
    BSONObj notMatchInt = BSON("x" << 1);
    BSONObj notMatchBool = BSON("x" << true);
    RegexMatchExpression regex;
    ASSERT(regex.init("", "1", "").isOK());
    ASSERT(!regex.matchesSingleElement(notMatchInt.firstElement()));
    ASSERT(!regex.matchesSingleElement(notMatchBool.firstElement()));
}

TEST(RegexMatchExpression, MatchesElementUtf8) {
    BSONObj multiByteCharacter = BSON("x"
                                      << "\xc2\xa5");
    RegexMatchExpression regex;
    ASSERT(regex.init("", "^.$", "").isOK());
    ASSERT(regex.matchesSingleElement(multiByteCharacter.firstElement()));
}

TEST(RegexMatchExpression, MatchesScalar) {
    RegexMatchExpression regex;
    ASSERT(regex.init("a", "b", "").isOK());
    ASSERT(regex.matchesBSON(BSON("a"
                                  << "b"),
                             NULL));
    ASSERT(!regex.matchesBSON(BSON("a"
                                   << "c"),
                              NULL));
}

TEST(RegexMatchExpression, MatchesArrayValue) {
    RegexMatchExpression regex;
    ASSERT(regex.init("a", "b", "").isOK());
    ASSERT(regex.matchesBSON(BSON("a" << BSON_ARRAY("c"
                                                    << "b")),
                             NULL));
    ASSERT(!regex.matchesBSON(BSON("a" << BSON_ARRAY("d"
                                                     << "c")),
                              NULL));
}

TEST(RegexMatchExpression, MatchesNull) {
    RegexMatchExpression regex;
    ASSERT(regex.init("a", "b", "").isOK());
    ASSERT(!regex.matchesBSON(BSONObj(), NULL));
    ASSERT(!regex.matchesBSON(BSON("a" << BSONNULL), NULL));
}

TEST(RegexMatchExpression, ElemMatchKey) {
    RegexMatchExpression regex;
    ASSERT(regex.init("a", "b", "").isOK());
    MatchDetails details;
    details.requestElemMatchKey();
    ASSERT(!regex.matchesBSON(BSON("a"
                                   << "c"),
                              &details));
    ASSERT(!details.hasElemMatchKey());
    ASSERT(regex.matchesBSON(BSON("a"
                                  << "b"),
                             &details));
    ASSERT(!details.hasElemMatchKey());
    ASSERT(regex.matchesBSON(BSON("a" << BSON_ARRAY("c"
                                                    << "b")),
                             &details));
    ASSERT(details.hasElemMatchKey());
    ASSERT_EQUALS("1", details.elemMatchKey());
}

TEST(RegexMatchExpression, Equality1) {
    RegexMatchExpression r1;
    RegexMatchExpression r2;
    RegexMatchExpression r3;
    RegexMatchExpression r4;
    ASSERT(r1.init("a", "b", "").isOK());
    ASSERT(r2.init("a", "b", "x").isOK());
    ASSERT(r3.init("a", "c", "").isOK());
    ASSERT(r4.init("b", "b", "").isOK());

    ASSERT(r1.equivalent(&r1));
    ASSERT(!r1.equivalent(&r2));
    ASSERT(!r1.equivalent(&r3));
    ASSERT(!r1.equivalent(&r4));
}

/**
   TEST( RegexMatchExpression, MatchesIndexKeyScalar ) {
   RegexMatchExpression regex;
   ASSERT( regex.init( "a", "xyz", "" ).isOK() );
   IndexSpec indexSpec( BSON( "a" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_True ==
   regex.matchesIndexKey( BSON( "" << "z xyz" ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_False ==
   regex.matchesIndexKey( BSON( "" << "xy" ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_False ==
   regex.matchesIndexKey( BSON( "" << BSON_ARRAY( "xyz" ) ), indexSpec ) );
   }

   TEST( RegexMatchExpression, MatchesIndexKeyMissing ) {
   RegexMatchExpression regex;
   ASSERT( regex.init( "a", "xyz", "" ).isOK() );
   IndexSpec indexSpec( BSON( "b" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   regex.matchesIndexKey( BSON( "" << "z xyz" ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   regex.matchesIndexKey( BSON( "" << "xy" ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   regex.matchesIndexKey( BSON( "" << BSON_ARRAY( 8 << "xyz" ) ), indexSpec ) );
   }

   TEST( RegexMatchExpression, MatchesIndexKeyArrayValue ) {
   RegexMatchExpression regex;
   ASSERT( regex.init( "a", "xyz", "" ).isOK() );
   IndexSpec indexSpec( BSON( "loc" << "mockarrayvalue" << "a" << 1 ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_True ==
   regex.matchesIndexKey( BSON( "" << "dummygeohash" << "" << "xyz" ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_False ==
   regex.matchesIndexKey( BSON( "" << "dummygeohash" << "" << "z" ), indexSpec ) );
   ASSERT( MatchMatchExpression::PartialMatchResult_True ==
   regex.matchesIndexKey( BSON( "" << "dummygeohash" <<
   "" << BSON_ARRAY( "r" << 6 << "xyz" ) ), indexSpec ) );
   }
*/

TEST(ModMatchExpression, MatchesElement) {
    BSONObj match = BSON("a" << 1);
    BSONObj largerMatch = BSON("a" << 4.0);
    BSONObj longLongMatch = BSON("a" << 68719476736LL);
    BSONObj notMatch = BSON("a" << 6);
    BSONObj negativeNotMatch = BSON("a" << -2);
    ModMatchExpression mod;
    ASSERT(mod.init("", 3, 1).isOK());
    ASSERT(mod.matchesSingleElement(match.firstElement()));
    ASSERT(mod.matchesSingleElement(largerMatch.firstElement()));
    ASSERT(mod.matchesSingleElement(longLongMatch.firstElement()));
    ASSERT(!mod.matchesSingleElement(notMatch.firstElement()));
    ASSERT(!mod.matchesSingleElement(negativeNotMatch.firstElement()));
}

TEST(ModMatchExpression, ZeroDivisor) {
    ModMatchExpression mod;
    ASSERT(!mod.init("", 0, 1).isOK());
}

TEST(ModMatchExpression, MatchesScalar) {
    ModMatchExpression mod;
    ASSERT(mod.init("a", 5, 2).isOK());
    ASSERT(mod.matchesBSON(BSON("a" << 7.0), NULL));
    ASSERT(!mod.matchesBSON(BSON("a" << 4), NULL));
}

TEST(ModMatchExpression, MatchesArrayValue) {
    ModMatchExpression mod;
    ASSERT(mod.init("a", 5, 2).isOK());
    ASSERT(mod.matchesBSON(BSON("a" << BSON_ARRAY(5 << 12LL)), NULL));
    ASSERT(!mod.matchesBSON(BSON("a" << BSON_ARRAY(6 << 8)), NULL));
}

TEST(ModMatchExpression, MatchesNull) {
    ModMatchExpression mod;
    ASSERT(mod.init("a", 5, 2).isOK());
    ASSERT(!mod.matchesBSON(BSONObj(), NULL));
    ASSERT(!mod.matchesBSON(BSON("a" << BSONNULL), NULL));
}

TEST(ModMatchExpression, ElemMatchKey) {
    ModMatchExpression mod;
    ASSERT(mod.init("a", 5, 2).isOK());
    MatchDetails details;
    details.requestElemMatchKey();
    ASSERT(!mod.matchesBSON(BSON("a" << 4), &details));
    ASSERT(!details.hasElemMatchKey());
    ASSERT(mod.matchesBSON(BSON("a" << 2), &details));
    ASSERT(!details.hasElemMatchKey());
    ASSERT(mod.matchesBSON(BSON("a" << BSON_ARRAY(1 << 2 << 5)), &details));
    ASSERT(details.hasElemMatchKey());
    ASSERT_EQUALS("1", details.elemMatchKey());
}

TEST(ModMatchExpression, Equality1) {
    ModMatchExpression m1;
    ModMatchExpression m2;
    ModMatchExpression m3;
    ModMatchExpression m4;

    m1.init("a", 1, 2);
    m2.init("a", 2, 2);
    m3.init("a", 1, 1);
    m4.init("b", 1, 2);

    ASSERT(m1.equivalent(&m1));
    ASSERT(!m1.equivalent(&m2));
    ASSERT(!m1.equivalent(&m3));
    ASSERT(!m1.equivalent(&m4));
}

/**
   TEST( ModMatchExpression, MatchesIndexKey ) {
   BSONObj operand = BSON( "$mod" << BSON_ARRAY( 2 << 1 ) );
   ModMatchExpression mod;
   ASSERT( mod.init( "a", operand[ "$mod" ] ).isOK() );
   IndexSpec indexSpec( BSON( "a" << 1 ) );
   BSONObj indexKey = BSON( "" << 1 );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   mod.matchesIndexKey( indexKey, indexSpec ) );
   }
*/

TEST(ExistsMatchExpression, MatchesElement) {
    BSONObj existsInt = BSON("a" << 5);
    BSONObj existsNull = BSON("a" << BSONNULL);
    BSONObj doesntExist = BSONObj();
    ExistsMatchExpression exists;
    ASSERT(exists.init("").isOK());
    ASSERT(exists.matchesSingleElement(existsInt.firstElement()));
    ASSERT(exists.matchesSingleElement(existsNull.firstElement()));
    ASSERT(!exists.matchesSingleElement(doesntExist.firstElement()));
}

TEST(ExistsMatchExpression, MatchesElementExistsTrueValue) {
    BSONObj exists = BSON("a" << 5);
    BSONObj missing = BSONObj();
    ExistsMatchExpression existsTrueValue;
    ASSERT(existsTrueValue.init("").isOK());
    ASSERT(existsTrueValue.matchesSingleElement(exists.firstElement()));
    ASSERT(!existsTrueValue.matchesSingleElement(missing.firstElement()));
}

TEST(ExistsMatchExpression, MatchesScalar) {
    ExistsMatchExpression exists;
    ASSERT(exists.init("a").isOK());
    ASSERT(exists.matchesBSON(BSON("a" << 1), NULL));
    ASSERT(exists.matchesBSON(BSON("a" << BSONNULL), NULL));
    ASSERT(!exists.matchesBSON(BSON("b" << 1), NULL));
}

TEST(ExistsMatchExpression, MatchesArray) {
    ExistsMatchExpression exists;
    ASSERT(exists.init("a").isOK());
    ASSERT(exists.matchesBSON(BSON("a" << BSON_ARRAY(4 << 5.5)), NULL));
}

TEST(ExistsMatchExpression, ElemMatchKey) {
    ExistsMatchExpression exists;
    ASSERT(exists.init("a.b").isOK());
    MatchDetails details;
    details.requestElemMatchKey();
    ASSERT(!exists.matchesBSON(BSON("a" << 1), &details));
    ASSERT(!details.hasElemMatchKey());
    ASSERT(exists.matchesBSON(BSON("a" << BSON("b" << 6)), &details));
    ASSERT(!details.hasElemMatchKey());
    ASSERT(exists.matchesBSON(BSON("a" << BSON_ARRAY(2 << BSON("b" << 7))), &details));
    ASSERT(details.hasElemMatchKey());
    ASSERT_EQUALS("1", details.elemMatchKey());
}

TEST(ExistsMatchExpression, Equivalent) {
    ExistsMatchExpression e1;
    ExistsMatchExpression e2;
    e1.init("a");
    e2.init("b");

    ASSERT(e1.equivalent(&e1));
    ASSERT(!e1.equivalent(&e2));
}

/**
   TEST( ExistsMatchExpression, MatchesIndexKey ) {
   BSONObj operand = BSON( "$exists" << true );
   ExistsMatchExpression exists;
   ASSERT( exists.init( "a", operand[ "$exists" ] ).isOK() );
   IndexSpec indexSpec( BSON( "a" << 1 ) );
   BSONObj indexKey = BSON( "" << 1 );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   exists.matchesIndexKey( indexKey, indexSpec ) );
   }
*/


TEST(TypeMatchExpression, MatchesElementStringType) {
    BSONObj match = BSON("a"
                         << "abc");
    BSONObj notMatch = BSON("a" << 5);
    TypeMatchExpression type;
    ASSERT(type.init("", String).isOK());
    ASSERT(type.matchesSingleElement(match["a"]));
    ASSERT(!type.matchesSingleElement(notMatch["a"]));
}

TEST(TypeMatchExpression, MatchesElementNullType) {
    BSONObj match = BSON("a" << BSONNULL);
    BSONObj notMatch = BSON("a"
                            << "abc");
    TypeMatchExpression type;
    ASSERT(type.init("", jstNULL).isOK());
    ASSERT(type.matchesSingleElement(match["a"]));
    ASSERT(!type.matchesSingleElement(notMatch["a"]));
}

TEST(TypeMatchExpression, InvalidTypeMatchExpressionerand) {
    // If the provided type number is not a valid BSONType, it is not a parse error.  The
    // operator will simply not match anything.
    BSONObj notMatch1 = BSON("a" << BSONNULL);
    BSONObj notMatch2 = BSON("a"
                             << "abc");
    TypeMatchExpression type;
    ASSERT(type.init("", JSTypeMax + 1).isOK());
    ASSERT(!type.matchesSingleElement(notMatch1["a"]));
    ASSERT(!type.matchesSingleElement(notMatch2["a"]));
}

TEST(TypeMatchExpression, MatchesScalar) {
    TypeMatchExpression type;
    ASSERT(type.init("a", Bool).isOK());
    ASSERT(type.matchesBSON(BSON("a" << true), NULL));
    ASSERT(!type.matchesBSON(BSON("a" << 1), NULL));
}

TEST(TypeMatchExpression, MatchesArray) {
    TypeMatchExpression type;
    ASSERT(type.init("a", NumberInt).isOK());
    ASSERT(type.matchesBSON(BSON("a" << BSON_ARRAY(4)), NULL));
    ASSERT(type.matchesBSON(BSON("a" << BSON_ARRAY(4 << "a")), NULL));
    ASSERT(type.matchesBSON(BSON("a" << BSON_ARRAY("a" << 4)), NULL));
    ASSERT(!type.matchesBSON(BSON("a" << BSON_ARRAY("a")), NULL));
    ASSERT(!type.matchesBSON(BSON("a" << BSON_ARRAY(BSON_ARRAY(4))), NULL));
}

TEST(TypeMatchExpression, MatchesOuterArray) {
    TypeMatchExpression type;
    ASSERT(type.init("a", Array).isOK());
    // The outer array is not matched.
    ASSERT(!type.matchesBSON(BSON("a" << BSONArray()), NULL));
    ASSERT(!type.matchesBSON(BSON("a" << BSON_ARRAY(4 << "a")), NULL));
    ASSERT(type.matchesBSON(BSON("a" << BSON_ARRAY(BSONArray() << 2)), NULL));
    ASSERT(!type.matchesBSON(BSON("a"
                                  << "bar"),
                             NULL));
}

TEST(TypeMatchExpression, MatchesObject) {
    TypeMatchExpression type;
    ASSERT(type.init("a", Object).isOK());
    ASSERT(type.matchesBSON(BSON("a" << BSON("b" << 1)), NULL));
    ASSERT(!type.matchesBSON(BSON("a" << 1), NULL));
}

TEST(TypeMatchExpression, MatchesDotNotationFieldObject) {
    TypeMatchExpression type;
    ASSERT(type.init("a.b", Object).isOK());
    ASSERT(type.matchesBSON(BSON("a" << BSON("b" << BSON("c" << 1))), NULL));
    ASSERT(!type.matchesBSON(BSON("a" << BSON("b" << 1)), NULL));
}

TEST(TypeMatchExpression, MatchesDotNotationArrayElementArray) {
    TypeMatchExpression type;
    ASSERT(type.init("a.0", Array).isOK());
    ASSERT(type.matchesBSON(BSON("a" << BSON_ARRAY(BSON_ARRAY(1))), NULL));
    ASSERT(!type.matchesBSON(BSON("a" << BSON_ARRAY("b")), NULL));
}

TEST(TypeMatchExpression, MatchesDotNotationArrayElementScalar) {
    TypeMatchExpression type;
    ASSERT(type.init("a.0", String).isOK());
    ASSERT(type.matchesBSON(BSON("a" << BSON_ARRAY("b")), NULL));
    ASSERT(!type.matchesBSON(BSON("a" << BSON_ARRAY(1)), NULL));
}

TEST(TypeMatchExpression, MatchesDotNotationArrayElementObject) {
    TypeMatchExpression type;
    ASSERT(type.init("a.0", Object).isOK());
    ASSERT(type.matchesBSON(BSON("a" << BSON_ARRAY(BSON("b" << 1))), NULL));
    ASSERT(!type.matchesBSON(BSON("a" << BSON_ARRAY(1)), NULL));
}

TEST(TypeMatchExpression, MatchesNull) {
    TypeMatchExpression type;
    ASSERT(type.init("a", jstNULL).isOK());
    ASSERT(type.matchesBSON(BSON("a" << BSONNULL), NULL));
    ASSERT(!type.matchesBSON(BSON("a" << 4), NULL));
    ASSERT(!type.matchesBSON(BSONObj(), NULL));
}

TEST(TypeMatchExpression, ElemMatchKey) {
    TypeMatchExpression type;
    ASSERT(type.init("a.b", String).isOK());
    MatchDetails details;
    details.requestElemMatchKey();
    ASSERT(!type.matchesBSON(BSON("a" << 1), &details));
    ASSERT(!details.hasElemMatchKey());
    ASSERT(type.matchesBSON(BSON("a" << BSON("b"
                                             << "string")),
                            &details));
    ASSERT(!details.hasElemMatchKey());
    ASSERT(type.matchesBSON(BSON("a" << BSON("b" << BSON_ARRAY("string"))), &details));
    ASSERT(details.hasElemMatchKey());
    ASSERT_EQUALS("0", details.elemMatchKey());
    ASSERT(type.matchesBSON(BSON("a" << BSON_ARRAY(2 << BSON("b" << BSON_ARRAY("string")))),
                            &details));
    ASSERT(details.hasElemMatchKey());
    ASSERT_EQUALS("1", details.elemMatchKey());
}

TEST(TypeMatchExpression, Equivalent) {
    TypeMatchExpression e1;
    TypeMatchExpression e2;
    TypeMatchExpression e3;
    e1.init("a", String);
    e2.init("a", NumberDouble);
    e3.init("b", String);

    ASSERT(e1.equivalent(&e1));
    ASSERT(!e1.equivalent(&e2));
    ASSERT(!e1.equivalent(&e3));
}


/**
   TEST( TypeMatchExpression, MatchesIndexKey ) {
   BSONObj operand = BSON( "$type" << 2 );
   TypeMatchExpression type;
   ASSERT( type.init( "a", operand[ "$type" ] ).isOK() );
   IndexSpec indexSpec( BSON( "a" << 1 ) );
   BSONObj indexKey = BSON( "" << "q" );
   ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
   type.matchesIndexKey( indexKey, indexSpec ) );
   }
*/


TEST(InMatchExpression, MatchesElementSingle) {
    BSONArray operand = BSON_ARRAY(1);
    BSONObj match = BSON("a" << 1);
    BSONObj notMatch = BSON("a" << 2);
    InMatchExpression in;
    in.getArrayFilterEntries()->addEquality(operand.firstElement());
    ASSERT(in.matchesSingleElement(match["a"]));
    ASSERT(!in.matchesSingleElement(notMatch["a"]));
}

TEST(InMatchExpression, MatchesEmpty) {
    InMatchExpression in;
    in.init("a");

    BSONObj notMatch = BSON("a" << 2);
    ASSERT(!in.matchesSingleElement(notMatch["a"]));
    ASSERT(!in.matchesBSON(BSON("a" << 1), NULL));
    ASSERT(!in.matchesBSON(BSONObj(), NULL));
}

TEST(InMatchExpression, MatchesElementMultiple) {
    BSONObj operand = BSON_ARRAY(1 << "r" << true << 1);
    InMatchExpression in;
    in.getArrayFilterEntries()->addEquality(operand[0]);
    in.getArrayFilterEntries()->addEquality(operand[1]);
    in.getArrayFilterEntries()->addEquality(operand[2]);
    in.getArrayFilterEntries()->addEquality(operand[3]);

    BSONObj matchFirst = BSON("a" << 1);
    BSONObj matchSecond = BSON("a"
                               << "r");
    BSONObj matchThird = BSON("a" << true);
    BSONObj notMatch = BSON("a" << false);
    ASSERT(in.matchesSingleElement(matchFirst["a"]));
    ASSERT(in.matchesSingleElement(matchSecond["a"]));
    ASSERT(in.matchesSingleElement(matchThird["a"]));
    ASSERT(!in.matchesSingleElement(notMatch["a"]));
}


TEST(InMatchExpression, MatchesScalar) {
    BSONObj operand = BSON_ARRAY(5);
    InMatchExpression in;
    in.init("a");
    in.getArrayFilterEntries()->addEquality(operand.firstElement());

    ASSERT(in.matchesBSON(BSON("a" << 5.0), NULL));
    ASSERT(!in.matchesBSON(BSON("a" << 4), NULL));
}

TEST(InMatchExpression, MatchesArrayValue) {
    BSONObj operand = BSON_ARRAY(5);
    InMatchExpression in;
    in.init("a");
    in.getArrayFilterEntries()->addEquality(operand.firstElement());

    ASSERT(in.matchesBSON(BSON("a" << BSON_ARRAY(5.0 << 6)), NULL));
    ASSERT(!in.matchesBSON(BSON("a" << BSON_ARRAY(6 << 7)), NULL));
    ASSERT(!in.matchesBSON(BSON("a" << BSON_ARRAY(BSON_ARRAY(5))), NULL));
}

TEST(InMatchExpression, MatchesNull) {
    BSONObj operand = BSON_ARRAY(BSONNULL);

    InMatchExpression in;
    in.init("a");
    in.getArrayFilterEntries()->addEquality(operand.firstElement());

    ASSERT(in.matchesBSON(BSONObj(), NULL));
    ASSERT(in.matchesBSON(BSON("a" << BSONNULL), NULL));
    ASSERT(!in.matchesBSON(BSON("a" << 4), NULL));
    // A non-existent field is treated same way as an empty bson object
    ASSERT(in.matchesBSON(BSON("b" << 4), NULL));
}

TEST(InMatchExpression, MatchesUndefined) {
    BSONObj operand = BSON_ARRAY(BSONUndefined);

    InMatchExpression in;
    in.init("a");
    Status s = in.getArrayFilterEntries()->addEquality(operand.firstElement());
    ASSERT_NOT_OK(s);
}

TEST(InMatchExpression, MatchesMinKey) {
    BSONObj operand = BSON_ARRAY(MinKey);
    InMatchExpression in;
    in.init("a");
    in.getArrayFilterEntries()->addEquality(operand.firstElement());

    ASSERT(in.matchesBSON(BSON("a" << MinKey), NULL));
    ASSERT(!in.matchesBSON(BSON("a" << MaxKey), NULL));
    ASSERT(!in.matchesBSON(BSON("a" << 4), NULL));
}

TEST(InMatchExpression, MatchesMaxKey) {
    BSONObj operand = BSON_ARRAY(MaxKey);
    InMatchExpression in;
    in.init("a");
    in.getArrayFilterEntries()->addEquality(operand.firstElement());

    ASSERT(in.matchesBSON(BSON("a" << MaxKey), NULL));
    ASSERT(!in.matchesBSON(BSON("a" << MinKey), NULL));
    ASSERT(!in.matchesBSON(BSON("a" << 4), NULL));
}

TEST(InMatchExpression, MatchesFullArray) {
    BSONObj operand = BSON_ARRAY(BSON_ARRAY(1 << 2) << 4 << 5);
    InMatchExpression in;
    in.init("a");
    in.getArrayFilterEntries()->addEquality(operand[0]);
    in.getArrayFilterEntries()->addEquality(operand[1]);
    in.getArrayFilterEntries()->addEquality(operand[2]);

    ASSERT(in.matchesBSON(BSON("a" << BSON_ARRAY(1 << 2)), NULL));
    ASSERT(!in.matchesBSON(BSON("a" << BSON_ARRAY(1 << 2 << 3)), NULL));
    ASSERT(!in.matchesBSON(BSON("a" << BSON_ARRAY(1)), NULL));
    ASSERT(!in.matchesBSON(BSON("a" << 1), NULL));
}

TEST(InMatchExpression, ElemMatchKey) {
    BSONObj operand = BSON_ARRAY(5 << 2);
    InMatchExpression in;
    in.init("a");
    in.getArrayFilterEntries()->addEquality(operand[0]);
    in.getArrayFilterEntries()->addEquality(operand[1]);

    MatchDetails details;
    details.requestElemMatchKey();
    ASSERT(!in.matchesBSON(BSON("a" << 4), &details));
    ASSERT(!details.hasElemMatchKey());
    ASSERT(in.matchesBSON(BSON("a" << 5), &details));
    ASSERT(!details.hasElemMatchKey());
    ASSERT(in.matchesBSON(BSON("a" << BSON_ARRAY(1 << 2 << 5)), &details));
    ASSERT(details.hasElemMatchKey());
    ASSERT_EQUALS("1", details.elemMatchKey());
}

/**
TEST( InMatchExpression, MatchesIndexKeyScalar ) {
    BSONObj operand = BSON( "$in" << BSON_ARRAY( 6 << 5 ) );
    InMatchExpression in;
    ASSERT( in.init( "a", operand[ "$in" ] ).isOK() );
    IndexSpec indexSpec( BSON( "a" << 1 ) );
    ASSERT( MatchMatchExpression::PartialMatchResult_True ==
            in.matchesIndexKey( BSON( "" << 6 ), indexSpec ) );
    ASSERT( MatchMatchExpression::PartialMatchResult_True ==
            in.matchesIndexKey( BSON( "" << 5 ), indexSpec ) );
    ASSERT( MatchMatchExpression::PartialMatchResult_False ==
            in.matchesIndexKey( BSON( "" << 4 ), indexSpec ) );
    ASSERT( MatchMatchExpression::PartialMatchResult_False ==
            in.matchesIndexKey( BSON( "" << BSON_ARRAY( 6 ) ), indexSpec ) );
}

TEST( InMatchExpression, MatchesIndexKeyMissing ) {
    BSONObj operand = BSON( "$in" << BSON_ARRAY( 6 ) );
    ComparisonMatchExpression eq
        ASSERT( eq.init( "a", operand[ "$in" ] ).isOK() );
    IndexSpec indexSpec( BSON( "b" << 1 ) );
    ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
            eq.matchesIndexKey( BSON( "" << 6 ), indexSpec ) );
    ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
            eq.matchesIndexKey( BSON( "" << 4 ), indexSpec ) );
    ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
            eq.matchesIndexKey( BSON( "" << BSON_ARRAY( 8 << 6 ) ), indexSpec ) );
}

TEST( InMatchExpression, MatchesIndexKeyArray ) {
    BSONObj operand = BSON( "$in" << BSON_ARRAY( 4 << BSON_ARRAY( 5 ) ) );
    InMatchExpression in;
    ASSERT( in.init( "a", operand[ "$in" ] ).isOK() );
    IndexSpec indexSpec( BSON( "a" << 1 ) );
    ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
            in.matchesIndexKey( BSON( "" << 4 ), indexSpec ) );
    ASSERT( MatchMatchExpression::PartialMatchResult_Unknown ==
            in.matchesIndexKey( BSON( "" << 5 ), indexSpec ) );
}

TEST( InMatchExpression, MatchesIndexKeyArrayValue ) {
    BSONObjBuilder inArray;
    inArray.append( "0", 4 ).append( "1", 5 ).appendRegex( "2", "abc", "" );
    BSONObj operand = BSONObjBuilder().appendArray( "$in", inArray.obj() ).obj();
    InMatchExpression in;
    ASSERT( in.init( "a", operand[ "$in" ] ).isOK() );
    IndexSpec indexSpec( BSON( "loc" << "mockarrayvalue" << "a" << 1 ) );
    ASSERT( MatchMatchExpression::PartialMatchResult_True ==
            in.matchesIndexKey( BSON( "" << "dummygeohash" << "" << 4 ), indexSpec ) );
    ASSERT( MatchMatchExpression::PartialMatchResult_False ==
            in.matchesIndexKey( BSON( "" << "dummygeohash" << "" << 6 ), indexSpec ) );
    ASSERT( MatchMatchExpression::PartialMatchResult_True ==
            in.matchesIndexKey( BSON( "" << "dummygeohash" << "" << "abcd" ), indexSpec ) );
    ASSERT( MatchMatchExpression::PartialMatchResult_True ==
            in.matchesIndexKey( BSONObjBuilder()
                                .append( "", "dummygeohash" )
                                .appendRegex( "", "abc", "" ).obj(),
                                indexSpec ) );
    ASSERT( MatchMatchExpression::PartialMatchResult_False ==
            in.matchesIndexKey( BSON( "" << "dummygeohash" << "" << "ab" ), indexSpec ) );
    ASSERT( MatchMatchExpression::PartialMatchResult_True ==
            in.matchesIndexKey( BSON( "" << "dummygeohash" <<
                                      "" << BSON_ARRAY( 8 << 5 ) ), indexSpec ) );
    ASSERT( MatchMatchExpression::PartialMatchResult_False ==
            in.matchesIndexKey( BSON( "" << "dummygeohash" <<
                                      "" << BSON_ARRAY( 8 << 9 ) ), indexSpec ) );
    ASSERT( MatchMatchExpression::PartialMatchResult_True ==
            in.matchesIndexKey( BSON( "" << "dummygeohash" <<
                                      "" << BSON_ARRAY( 8 << "abc" ) ), indexSpec ) );
    ASSERT( MatchMatchExpression::PartialMatchResult_False ==
            in.matchesIndexKey( BSON( "" << "dummygeohash" <<
                                      "" << BSON_ARRAY( 8 << "ac" ) ), indexSpec ) );
}
*/
}
