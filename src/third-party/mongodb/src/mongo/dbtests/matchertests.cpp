// matchertests.cpp : matcher unit tests
//

/**
 *    Copyright (C) 2008 10gen Inc.
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

#include <iostream>

#include "mongo/db/json.h"
#include "mongo/db/matcher/matcher.h"
#include "mongo/db/operation_context_impl.h"
#include "mongo/dbtests/dbtests.h"
#include "mongo/util/timer.h"

namespace MatcherTests {

using std::cout;
using std::endl;
using std::string;

class CollectionBase {
public:
    CollectionBase() {}

    virtual ~CollectionBase() {}
};

template <typename M>
class Basic {
public:
    void run() {
        BSONObj query = fromjson("{\"a\":\"b\"}");
        M m(query, MatchExpressionParser::WhereCallback());
        ASSERT(m.matches(fromjson("{\"a\":\"b\"}")));
    }
};

template <typename M>
class DoubleEqual {
public:
    void run() {
        BSONObj query = fromjson("{\"a\":5}");
        M m(query, MatchExpressionParser::WhereCallback());
        ASSERT(m.matches(fromjson("{\"a\":5}")));
    }
};

template <typename M>
class MixedNumericEqual {
public:
    void run() {
        BSONObjBuilder query;
        query.append("a", 5);
        M m(query.done(), MatchExpressionParser::WhereCallback());
        ASSERT(m.matches(fromjson("{\"a\":5}")));
    }
};

template <typename M>
class MixedNumericGt {
public:
    void run() {
        BSONObj query = fromjson("{\"a\":{\"$gt\":4}}");
        M m(query, MatchExpressionParser::WhereCallback());
        BSONObjBuilder b;
        b.append("a", 5);
        ASSERT(m.matches(b.done()));
    }
};

template <typename M>
class MixedNumericIN {
public:
    void run() {
        BSONObj query = fromjson("{ a : { $in : [4,6] } }");
        ASSERT_EQUALS(4, query["a"].embeddedObject()["$in"].embeddedObject()["0"].number());
        ASSERT_EQUALS(NumberInt, query["a"].embeddedObject()["$in"].embeddedObject()["0"].type());

        M m(query, MatchExpressionParser::WhereCallback());

        {
            BSONObjBuilder b;
            b.append("a", 4.0);
            ASSERT(m.matches(b.done()));
        }

        {
            BSONObjBuilder b;
            b.append("a", 5);
            ASSERT(!m.matches(b.done()));
        }


        {
            BSONObjBuilder b;
            b.append("a", 4);
            ASSERT(m.matches(b.done()));
        }
    }
};

template <typename M>
class MixedNumericEmbedded {
public:
    void run() {
        M m(BSON("a" << BSON("x" << 1)), MatchExpressionParser::WhereCallback());
        ASSERT(m.matches(BSON("a" << BSON("x" << 1))));
        ASSERT(m.matches(BSON("a" << BSON("x" << 1.0))));
    }
};

template <typename M>
class Size {
public:
    void run() {
        M m(fromjson("{a:{$size:4}}"), MatchExpressionParser::WhereCallback());
        ASSERT(m.matches(fromjson("{a:[1,2,3,4]}")));
        ASSERT(!m.matches(fromjson("{a:[1,2,3]}")));
        ASSERT(!m.matches(fromjson("{a:[1,2,3,'a','b']}")));
        ASSERT(!m.matches(fromjson("{a:[[1,2,3,4]]}")));
    }
};

template <typename M>
class WithinBox {
public:
    void run() {
        M m(fromjson("{loc:{$within:{$box:[{x: 4, y:4},[6,6]]}}}"),
            MatchExpressionParser::WhereCallback());
        ASSERT(!m.matches(fromjson("{loc: [3,4]}")));
        ASSERT(m.matches(fromjson("{loc: [4,4]}")));
        ASSERT(m.matches(fromjson("{loc: [5,5]}")));
        ASSERT(m.matches(fromjson("{loc: [5,5.1]}")));
        ASSERT(m.matches(fromjson("{loc: {x: 5, y:5.1}}")));
    }
};

template <typename M>
class WithinPolygon {
public:
    void run() {
        M m(fromjson("{loc:{$within:{$polygon:[{x:0,y:0},[0,5],[5,5],[5,0]]}}}"),
            MatchExpressionParser::WhereCallback());
        ASSERT(m.matches(fromjson("{loc: [3,4]}")));
        ASSERT(m.matches(fromjson("{loc: [4,4]}")));
        ASSERT(m.matches(fromjson("{loc: {x:5,y:5}}")));
        ASSERT(!m.matches(fromjson("{loc: [5,5.1]}")));
        ASSERT(!m.matches(fromjson("{loc: {}}")));
    }
};

template <typename M>
class WithinCenter {
public:
    void run() {
        M m(fromjson("{loc:{$within:{$center:[{x:30,y:30},10]}}}"),
            MatchExpressionParser::WhereCallback());
        ASSERT(!m.matches(fromjson("{loc: [3,4]}")));
        ASSERT(m.matches(fromjson("{loc: {x:30,y:30}}")));
        ASSERT(m.matches(fromjson("{loc: [20,30]}")));
        ASSERT(m.matches(fromjson("{loc: [30,20]}")));
        ASSERT(m.matches(fromjson("{loc: [40,30]}")));
        ASSERT(m.matches(fromjson("{loc: [30,40]}")));
        ASSERT(!m.matches(fromjson("{loc: [31,40]}")));
    }
};

/** Test that MatchDetails::elemMatchKey() is set correctly after a match. */
template <typename M>
class ElemMatchKey {
public:
    void run() {
        M matcher(BSON("a.b" << 1), MatchExpressionParser::WhereCallback());
        MatchDetails details;
        details.requestElemMatchKey();
        ASSERT(!details.hasElemMatchKey());
        ASSERT(matcher.matches(fromjson("{ a:[ { b:1 } ] }"), &details));
        // The '0' entry of the 'a' array is matched.
        ASSERT(details.hasElemMatchKey());
        ASSERT_EQUALS(string("0"), details.elemMatchKey());
    }
};

template <typename M>
class WhereSimple1 {
public:
    void run() {
        OperationContextImpl txn;
        AutoGetCollectionForRead ctx(&txn, "unittests.matchertests");

        M m(BSON("$where"
                 << "function(){ return this.a == 1; }"),
            WhereCallbackReal(&txn, StringData("unittests")));
        ASSERT(m.matches(BSON("a" << 1)));
        ASSERT(!m.matches(BSON("a" << 2)));
    }
};

template <typename M>
class TimingBase {
public:
    long dotime(const BSONObj& patt, const BSONObj& obj) {
        M m(patt, MatchExpressionParser::WhereCallback());
        Timer t;
        for (int i = 0; i < 900000; i++) {
            if (!m.matches(obj)) {
                ASSERT(0);
            }
        }
        return t.millis();
    }
};

template <typename M>
class AllTiming : public TimingBase<M> {
public:
    void run() {
        long normal = TimingBase<M>::dotime(BSON("x" << 5), BSON("x" << 5));

        long all =
            TimingBase<M>::dotime(BSON("x" << BSON("$all" << BSON_ARRAY(5))), BSON("x" << 5));

        cout << "AllTiming " << demangleName(typeid(M)) << " normal: " << normal << " all: " << all
             << endl;
    }
};


class All : public Suite {
public:
    All() : Suite("matcher") {}

#define ADD_BOTH(TEST) add<TEST<Matcher>>();

    void setupTests() {
        ADD_BOTH(Basic);
        ADD_BOTH(DoubleEqual);
        ADD_BOTH(MixedNumericEqual);
        ADD_BOTH(MixedNumericGt);
        ADD_BOTH(MixedNumericIN);
        ADD_BOTH(Size);
        ADD_BOTH(MixedNumericEmbedded);
        ADD_BOTH(ElemMatchKey);
        ADD_BOTH(WhereSimple1);
        ADD_BOTH(AllTiming);
        ADD_BOTH(WithinBox);
        ADD_BOTH(WithinCenter);
        ADD_BOTH(WithinPolygon);
    }
};

SuiteInstance<All> dball;

}  // namespace MatcherTests
