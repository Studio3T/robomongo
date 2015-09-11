/**
 * Copyright (c) 2011 10gen Inc.
 *
 * This program is free software: you can redistribute it and/or  modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, the copyright holders give permission to link the
 * code of portions of this program with the OpenSSL library under certain
 * conditions as described in each individual source file and distribute
 * linked combinations including the program with the OpenSSL library. You
 * must comply with the GNU Affero General Public License in all respects for
 * all of the code used other than as permitted herein. If you modify file(s)
 * with this exception, you may extend this exception to your version of the
 * file(s), but you are not obligated to do so. If you do not wish to do so,
 * delete this exception statement from your version. If you delete this
 * exception statement from all source files in the program, then also delete
 * it in the license file.
 */

#include "mongo/platform/basic.h"

#include "mongo/db/pipeline/expression.h"

#include <boost/algorithm/string.hpp>
#include <boost/preprocessor/cat.hpp>  // like the ## operator but works with __LINE__
#include <cstdio>

#include "mongo/base/init.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/pipeline/document.h"
#include "mongo/db/pipeline/expression_context.h"
#include "mongo/db/pipeline/value.h"
#include "mongo/stdx/functional.h"
#include "mongo/util/string_map.h"
#include "mongo/util/mongoutils/str.h"

namespace mongo {
using namespace mongoutils;

using boost::intrusive_ptr;
using std::set;
using std::string;
using std::vector;

/// Helper function to easily wrap constants with $const.
static Value serializeConstant(Value val) {
    return Value(DOC("$const" << val));
}

void Variables::uassertValidNameForUserWrite(StringData varName) {
    // System variables users allowed to write to (currently just one)
    if (varName == "CURRENT") {
        return;
    }

    uassert(16866, "empty variable names are not allowed", !varName.empty());

    const bool firstCharIsValid =
        (varName[0] >= 'a' && varName[0] <= 'z') || (varName[0] & '\x80')  // non-ascii
        ;

    uassert(16867,
            str::stream() << "'" << varName
                          << "' starts with an invalid character for a user variable name",
            firstCharIsValid);

    for (size_t i = 1; i < varName.size(); i++) {
        const bool charIsValid = (varName[i] >= 'a' && varName[i] <= 'z') ||
            (varName[i] >= 'A' && varName[i] <= 'Z') || (varName[i] >= '0' && varName[i] <= '9') ||
            (varName[i] == '_') || (varName[i] & '\x80')  // non-ascii
            ;

        uassert(16868,
                str::stream() << "'" << varName << "' contains an invalid character "
                              << "for a variable name: '" << varName[i] << "'",
                charIsValid);
    }
}

void Variables::uassertValidNameForUserRead(StringData varName) {
    uassert(16869, "empty variable names are not allowed", !varName.empty());

    const bool firstCharIsValid = (varName[0] >= 'a' && varName[0] <= 'z') ||
        (varName[0] >= 'A' && varName[0] <= 'Z') || (varName[0] & '\x80')  // non-ascii
        ;

    uassert(16870,
            str::stream() << "'" << varName
                          << "' starts with an invalid character for a variable name",
            firstCharIsValid);

    for (size_t i = 1; i < varName.size(); i++) {
        const bool charIsValid = (varName[i] >= 'a' && varName[i] <= 'z') ||
            (varName[i] >= 'A' && varName[i] <= 'Z') || (varName[i] >= '0' && varName[i] <= '9') ||
            (varName[i] == '_') || (varName[i] & '\x80')  // non-ascii
            ;

        uassert(16871,
                str::stream() << "'" << varName << "' contains an invalid character "
                              << "for a variable name: '" << varName[i] << "'",
                charIsValid);
    }
}

void Variables::setValue(Id id, const Value& value) {
    massert(17199, "can't use Variables::setValue to set ROOT", id != ROOT_ID);

    verify(id < _numVars);
    _rest[id] = value;
}

Value Variables::getValue(Id id) const {
    if (id == ROOT_ID)
        return Value(_root);

    verify(id < _numVars);
    return _rest[id];
}

Document Variables::getDocument(Id id) const {
    if (id == ROOT_ID)
        return _root;

    verify(id < _numVars);
    const Value var = _rest[id];
    if (var.getType() == Object)
        return var.getDocument();

    return Document();
}

Variables::Id VariablesParseState::defineVariable(const StringData& name) {
    // caller should have validated before hand by using Variables::uassertValidNameForUserWrite
    massert(17275, "Can't redefine ROOT", name != "ROOT");

    Variables::Id id = _idGenerator->generateId();
    _variables[name] = id;
    return id;
}

Variables::Id VariablesParseState::getVariable(const StringData& name) const {
    StringMap<Variables::Id>::const_iterator it = _variables.find(name);
    if (it != _variables.end())
        return it->second;

    uassert(17276,
            str::stream() << "Use of undefined variable: " << name,
            name == "ROOT" || name == "CURRENT");

    return Variables::ROOT_ID;
}

/* --------------------------- Expression ------------------------------ */

Expression::ObjectCtx::ObjectCtx(int theOptions) : options(theOptions) {}

bool Expression::ObjectCtx::documentOk() const {
    return ((options & DOCUMENT_OK) != 0);
}

bool Expression::ObjectCtx::topLevel() const {
    return ((options & TOP_LEVEL) != 0);
}

bool Expression::ObjectCtx::inclusionOk() const {
    return ((options & INCLUSION_OK) != 0);
}

string Expression::removeFieldPrefix(const string& prefixedField) {
    uassert(16419,
            str::stream() << "field path must not contain embedded null characters"
                          << prefixedField.find("\0") << ",",
            prefixedField.find('\0') == string::npos);

    const char* pPrefixedField = prefixedField.c_str();
    uassert(15982,
            str::stream() << "field path references must be prefixed with a '$' ('" << prefixedField
                          << "'",
            pPrefixedField[0] == '$');

    return string(pPrefixedField + 1);
}

intrusive_ptr<Expression> Expression::parseObject(BSONObj obj,
                                                  ObjectCtx* pCtx,
                                                  const VariablesParseState& vps) {
    /*
      An object expression can take any of the following forms:

      f0: {f1: ..., f2: ..., f3: ...}
      f0: {$operator:[operand1, operand2, ...]}
    */

    intrusive_ptr<Expression> pExpression;              // the result
    intrusive_ptr<ExpressionObject> pExpressionObject;  // alt result
    enum { UNKNOWN, NOTOPERATOR, OPERATOR } kind = UNKNOWN;

    if (obj.isEmpty())
        return ExpressionObject::create();
    BSONObjIterator iter(obj);

    for (size_t fieldCount = 0; iter.more(); ++fieldCount) {
        BSONElement fieldElement(iter.next());
        const char* pFieldName = fieldElement.fieldName();

        if (pFieldName[0] == '$') {
            uassert(
                15983,
                str::stream() << "the operator must be the only field in a pipeline object (at '"
                              << pFieldName << "'",
                fieldCount == 0);

            uassert(16404,
                    "$expressions are not allowed at the top-level of $project",
                    !pCtx->topLevel());

            /* we've determined this "object" is an operator expression */
            kind = OPERATOR;

            pExpression = parseExpression(fieldElement, vps);
        } else {
            uassert(15990,
                    str::stream() << "this object is already an operator expression, and can't be "
                                     "used as a document expression (at '" << pFieldName << "')",
                    kind != OPERATOR);

            uassert(16405,
                    "dotted field names are only allowed at the top level",
                    pCtx->topLevel() || !str::contains(pFieldName, '.'));

            /* if it's our first time, create the document expression */
            if (!pExpression.get()) {
                verify(pCtx->documentOk());
                // CW TODO error: document not allowed in this context

                pExpressionObject =
                    pCtx->topLevel() ? ExpressionObject::createRoot() : ExpressionObject::create();
                pExpression = pExpressionObject;

                /* this "object" is not an operator expression */
                kind = NOTOPERATOR;
            }

            BSONType fieldType = fieldElement.type();
            string fieldName(pFieldName);
            switch (fieldType) {
                case Object: {
                    /* it's a nested document */
                    ObjectCtx oCtx((pCtx->documentOk() ? ObjectCtx::DOCUMENT_OK : 0) |
                                   (pCtx->inclusionOk() ? ObjectCtx::INCLUSION_OK : 0));

                    pExpressionObject->addField(fieldName,
                                                parseObject(fieldElement.Obj(), &oCtx, vps));
                    break;
                }
                case String: {
                    /* it's a renamed field */
                    // CW TODO could also be a constant
                    pExpressionObject->addField(
                        fieldName, ExpressionFieldPath::parse(fieldElement.str(), vps));
                    break;
                }
                case Bool:
                case NumberDouble:
                case NumberLong:
                case NumberInt: {
                    /* it's an inclusion specification */
                    if (fieldElement.trueValue()) {
                        uassert(16420,
                                "field inclusion is not allowed inside of $expressions",
                                pCtx->inclusionOk());
                        pExpressionObject->includePath(fieldName);
                    } else {
                        uassert(16406,
                                "The top-level _id field is the only field currently supported for "
                                "exclusion",
                                pCtx->topLevel() && fieldName == "_id");
                        pExpressionObject->excludeId(true);
                    }
                    break;
                }
                default:
                    uassert(15992,
                            str::stream() << "disallowed field type " << typeName(fieldType)
                                          << " in object expression (at '" << fieldName << "')",
                            false);
            }
        }
    }

    return pExpression;
}

namespace {
typedef stdx::function<intrusive_ptr<Expression>(BSONElement, const VariablesParseState&)>
    ExpressionParser;
StringMap<ExpressionParser> expressionParserMap;
}

/** Registers an ExpressionParser so it can be called from parseExpression and friends.
 *
 *  As an example, if your expression looks like {"$foo": [1,2,3]} you would add this line:
 *  REGISTER_EXPRESSION("$foo", ExpressionFoo::parse);
 */
#define REGISTER_EXPRESSION(key, parserFunc)                                                     \
    MONGO_INITIALIZER(BOOST_PP_CAT(addToExpressionParserMap, __LINE__))(InitializerContext*) {   \
        /* prevent duplicate expressions */                                                      \
        StringMap<ExpressionParser>::const_iterator op = expressionParserMap.find(key);          \
        massert(17064,                                                                           \
                str::stream() << "Duplicate expression (" << key << ") detected at " << __FILE__ \
                              << ":" << __LINE__,                                                \
                op == expressionParserMap.end());                                                \
        /* register expression */                                                                \
        expressionParserMap[key] = (parserFunc);                                                 \
        return Status::OK();                                                                     \
    }

intrusive_ptr<Expression> Expression::parseExpression(BSONElement exprElement,
                                                      const VariablesParseState& vps) {
    /* look for the specified operator */
    const char* opName = exprElement.fieldName();
    StringMap<ExpressionParser>::const_iterator op = expressionParserMap.find(opName);
    uassert(15999,
            str::stream() << "invalid operator '" << opName << "'",
            op != expressionParserMap.end());

    /* make the expression node */
    return op->second(exprElement, vps);
}

Expression::ExpressionVector ExpressionNary::parseArguments(BSONElement exprElement,
                                                            const VariablesParseState& vps) {
    ExpressionVector out;
    if (exprElement.type() == Array) {
        BSONForEach(elem, exprElement.Obj()) {
            out.push_back(Expression::parseOperand(elem, vps));
        }
    } else {  // assume it's an atomic operand
        out.push_back(Expression::parseOperand(exprElement, vps));
    }

    return out;
}

intrusive_ptr<Expression> Expression::parseOperand(BSONElement exprElement,
                                                   const VariablesParseState& vps) {
    BSONType type = exprElement.type();

    if (type == String && exprElement.valuestr()[0] == '$') {
        /* if we got here, this is a field path expression */
        return ExpressionFieldPath::parse(exprElement.str(), vps);
    } else if (type == Object) {
        ObjectCtx oCtx(ObjectCtx::DOCUMENT_OK);
        return Expression::parseObject(exprElement.Obj(), &oCtx, vps);
    } else {
        return ExpressionConstant::parse(exprElement, vps);
    }
}

/* ------------------------- ExpressionAdd ----------------------------- */

Value ExpressionAdd::evaluateInternal(Variables* vars) const {
    /*
      We'll try to return the narrowest possible result value.  To do that
      without creating intermediate Values, do the arithmetic for double
      and integral types in parallel, tracking the current narrowest
      type.
     */
    double doubleTotal = 0;
    long long longTotal = 0;
    BSONType totalType = NumberInt;
    bool haveDate = false;

    const size_t n = vpOperand.size();
    for (size_t i = 0; i < n; ++i) {
        Value val = vpOperand[i]->evaluateInternal(vars);

        if (val.numeric()) {
            totalType = Value::getWidestNumeric(totalType, val.getType());

            doubleTotal += val.coerceToDouble();
            longTotal += val.coerceToLong();
        } else if (val.getType() == Date) {
            uassert(16612, "only one Date allowed in an $add expression", !haveDate);
            haveDate = true;

            // We don't manipulate totalType here.

            longTotal += val.getDate();
            doubleTotal += val.getDate();
        } else if (val.nullish()) {
            return Value(BSONNULL);
        } else {
            uasserted(16554,
                      str::stream() << "$add only supports numeric or date types, not "
                                    << typeName(val.getType()));
        }
    }

    if (haveDate) {
        if (totalType == NumberDouble)
            longTotal = static_cast<long long>(doubleTotal);
        return Value(Date_t(longTotal));
    } else if (totalType == NumberLong) {
        return Value(longTotal);
    } else if (totalType == NumberDouble) {
        return Value(doubleTotal);
    } else if (totalType == NumberInt) {
        return Value::createIntOrLong(longTotal);
    } else {
        massert(16417, "$add resulted in a non-numeric type", false);
    }
}

REGISTER_EXPRESSION("$add", ExpressionAdd::parse);
const char* ExpressionAdd::getOpName() const {
    return "$add";
}

/* ------------------------- ExpressionAllElementsTrue -------------------------- */

Value ExpressionAllElementsTrue::evaluateInternal(Variables* vars) const {
    const Value arr = vpOperand[0]->evaluateInternal(vars);
    uassert(17040,
            str::stream() << getOpName() << "'s argument must be an array, but is "
                          << typeName(arr.getType()),
            arr.getType() == Array);
    const vector<Value>& array = arr.getArray();
    for (vector<Value>::const_iterator it = array.begin(); it != array.end(); ++it) {
        if (!it->coerceToBool()) {
            return Value(false);
        }
    }
    return Value(true);
}

REGISTER_EXPRESSION("$allElementsTrue", ExpressionAllElementsTrue::parse);
const char* ExpressionAllElementsTrue::getOpName() const {
    return "$allElementsTrue";
}

/* ------------------------- ExpressionAnd ----------------------------- */

intrusive_ptr<Expression> ExpressionAnd::optimize() {
    /* optimize the conjunction as much as possible */
    intrusive_ptr<Expression> pE(ExpressionNary::optimize());

    /* if the result isn't a conjunction, we can't do anything */
    ExpressionAnd* pAnd = dynamic_cast<ExpressionAnd*>(pE.get());
    if (!pAnd)
        return pE;

    /*
      Check the last argument on the result; if it's not constant (as
      promised by ExpressionNary::optimize(),) then there's nothing
      we can do.
    */
    const size_t n = pAnd->vpOperand.size();
    // ExpressionNary::optimize() generates an ExpressionConstant for {$and:[]}.
    verify(n > 0);
    intrusive_ptr<Expression> pLast(pAnd->vpOperand[n - 1]);
    const ExpressionConstant* pConst = dynamic_cast<ExpressionConstant*>(pLast.get());
    if (!pConst)
        return pE;

    /*
      Evaluate and coerce the last argument to a boolean.  If it's false,
      then we can replace this entire expression.
     */
    bool last = pConst->getValue().coerceToBool();
    if (!last) {
        intrusive_ptr<ExpressionConstant> pFinal(ExpressionConstant::create(Value(false)));
        return pFinal;
    }

    /*
      If we got here, the final operand was true, so we don't need it
      anymore.  If there was only one other operand, we don't need the
      conjunction either.  Note we still need to keep the promise that
      the result will be a boolean.
     */
    if (n == 2) {
        intrusive_ptr<Expression> pFinal(ExpressionCoerceToBool::create(pAnd->vpOperand[0]));
        return pFinal;
    }

    /*
      Remove the final "true" value, and return the new expression.

      CW TODO:
      Note that because of any implicit conversions, we may need to
      apply an implicit boolean conversion.
    */
    pAnd->vpOperand.resize(n - 1);
    return pE;
}

Value ExpressionAnd::evaluateInternal(Variables* vars) const {
    const size_t n = vpOperand.size();
    for (size_t i = 0; i < n; ++i) {
        Value pValue(vpOperand[i]->evaluateInternal(vars));
        if (!pValue.coerceToBool())
            return Value(false);
    }

    return Value(true);
}

REGISTER_EXPRESSION("$and", ExpressionAnd::parse);
const char* ExpressionAnd::getOpName() const {
    return "$and";
}

/* ------------------------- ExpressionAnyElementTrue -------------------------- */

Value ExpressionAnyElementTrue::evaluateInternal(Variables* vars) const {
    const Value arr = vpOperand[0]->evaluateInternal(vars);
    uassert(17041,
            str::stream() << getOpName() << "'s argument must be an array, but is "
                          << typeName(arr.getType()),
            arr.getType() == Array);
    const vector<Value>& array = arr.getArray();
    for (vector<Value>::const_iterator it = array.begin(); it != array.end(); ++it) {
        if (it->coerceToBool()) {
            return Value(true);
        }
    }
    return Value(false);
}

REGISTER_EXPRESSION("$anyElementTrue", ExpressionAnyElementTrue::parse);
const char* ExpressionAnyElementTrue::getOpName() const {
    return "$anyElementTrue";
}

/* -------------------- ExpressionCoerceToBool ------------------------- */

intrusive_ptr<ExpressionCoerceToBool> ExpressionCoerceToBool::create(
    const intrusive_ptr<Expression>& pExpression) {
    intrusive_ptr<ExpressionCoerceToBool> pNew(new ExpressionCoerceToBool(pExpression));
    return pNew;
}

ExpressionCoerceToBool::ExpressionCoerceToBool(const intrusive_ptr<Expression>& pTheExpression)
    : Expression(), pExpression(pTheExpression) {}

intrusive_ptr<Expression> ExpressionCoerceToBool::optimize() {
    /* optimize the operand */
    pExpression = pExpression->optimize();

    /* if the operand already produces a boolean, then we don't need this */
    /* LATER - Expression to support a "typeof" query? */
    Expression* pE = pExpression.get();
    if (dynamic_cast<ExpressionAnd*>(pE) || dynamic_cast<ExpressionOr*>(pE) ||
        dynamic_cast<ExpressionNot*>(pE) || dynamic_cast<ExpressionCoerceToBool*>(pE))
        return pExpression;

    return intrusive_ptr<Expression>(this);
}

void ExpressionCoerceToBool::addDependencies(DepsTracker* deps, vector<string>* path) const {
    pExpression->addDependencies(deps);
}

Value ExpressionCoerceToBool::evaluateInternal(Variables* vars) const {
    Value pResult(pExpression->evaluateInternal(vars));
    bool b = pResult.coerceToBool();
    if (b)
        return Value(true);
    return Value(false);
}

Value ExpressionCoerceToBool::serialize(bool explain) const {
    // When not explaining, serialize to an $and expression. When parsed, the $and expression
    // will be optimized back into a ExpressionCoerceToBool.
    const char* name = explain ? "$coerceToBool" : "$and";
    return Value(DOC(name << DOC_ARRAY(pExpression->serialize(explain))));
}

/* ----------------------- ExpressionCompare --------------------------- */

REGISTER_EXPRESSION("$cmp",
                    stdx::bind(ExpressionCompare::parse,
                               stdx::placeholders::_1,
                               stdx::placeholders::_2,
                               ExpressionCompare::CMP));
REGISTER_EXPRESSION("$eq",
                    stdx::bind(ExpressionCompare::parse,
                               stdx::placeholders::_1,
                               stdx::placeholders::_2,
                               ExpressionCompare::EQ));
REGISTER_EXPRESSION("$gt",
                    stdx::bind(ExpressionCompare::parse,
                               stdx::placeholders::_1,
                               stdx::placeholders::_2,
                               ExpressionCompare::GT));
REGISTER_EXPRESSION("$gte",
                    stdx::bind(ExpressionCompare::parse,
                               stdx::placeholders::_1,
                               stdx::placeholders::_2,
                               ExpressionCompare::GTE));
REGISTER_EXPRESSION("$lt",
                    stdx::bind(ExpressionCompare::parse,
                               stdx::placeholders::_1,
                               stdx::placeholders::_2,
                               ExpressionCompare::LT));
REGISTER_EXPRESSION("$lte",
                    stdx::bind(ExpressionCompare::parse,
                               stdx::placeholders::_1,
                               stdx::placeholders::_2,
                               ExpressionCompare::LTE));
REGISTER_EXPRESSION("$ne",
                    stdx::bind(ExpressionCompare::parse,
                               stdx::placeholders::_1,
                               stdx::placeholders::_2,
                               ExpressionCompare::NE));
intrusive_ptr<Expression> ExpressionCompare::parse(BSONElement bsonExpr,
                                                   const VariablesParseState& vps,
                                                   CmpOp op) {
    intrusive_ptr<ExpressionCompare> expr = new ExpressionCompare(op);
    ExpressionVector args = parseArguments(bsonExpr, vps);
    expr->validateArguments(args);
    expr->vpOperand = args;
    return expr;
}

ExpressionCompare::ExpressionCompare(CmpOp theCmpOp) : cmpOp(theCmpOp) {}

namespace {
// Lookup table for truth value returns
struct CmpLookup {
    const bool truthValue[3];                // truth value for -1, 0, 1
    const ExpressionCompare::CmpOp reverse;  // reverse(b,a) returns the same as op(a,b)
    const char name[5];                      // string name with trailing '\0'
};
static const CmpLookup cmpLookup[7] = {
    /*             -1      0      1      reverse                  name   */
    /* EQ  */ {{false, true, false}, ExpressionCompare::EQ, "$eq"},
    /* NE  */ {{true, false, true}, ExpressionCompare::NE, "$ne"},
    /* GT  */ {{false, false, true}, ExpressionCompare::LT, "$gt"},
    /* GTE */ {{false, true, true}, ExpressionCompare::LTE, "$gte"},
    /* LT  */ {{true, false, false}, ExpressionCompare::GT, "$lt"},
    /* LTE */ {{true, true, false}, ExpressionCompare::GTE, "$lte"},

    // CMP is special. Only name is used.
    /* CMP */ {{false, false, false}, ExpressionCompare::CMP, "$cmp"},
};
}

Value ExpressionCompare::evaluateInternal(Variables* vars) const {
    Value pLeft(vpOperand[0]->evaluateInternal(vars));
    Value pRight(vpOperand[1]->evaluateInternal(vars));

    int cmp = Value::compare(pLeft, pRight);

    // Make cmp one of 1, 0, or -1.
    if (cmp == 0) {
        // leave as 0
    } else if (cmp < 0) {
        cmp = -1;
    } else if (cmp > 0) {
        cmp = 1;
    }

    if (cmpOp == CMP)
        return Value(cmp);

    bool returnValue = cmpLookup[cmpOp].truthValue[cmp + 1];
    return Value(returnValue);
}

const char* ExpressionCompare::getOpName() const {
    return cmpLookup[cmpOp].name;
}

/* ------------------------- ExpressionConcat ----------------------------- */

Value ExpressionConcat::evaluateInternal(Variables* vars) const {
    const size_t n = vpOperand.size();

    StringBuilder result;
    for (size_t i = 0; i < n; ++i) {
        Value val = vpOperand[i]->evaluateInternal(vars);
        if (val.nullish())
            return Value(BSONNULL);

        uassert(16702,
                str::stream() << "$concat only supports strings, not " << typeName(val.getType()),
                val.getType() == String);

        result << val.coerceToString();
    }

    return Value(result.str());
}

REGISTER_EXPRESSION("$concat", ExpressionConcat::parse);
const char* ExpressionConcat::getOpName() const {
    return "$concat";
}

/* ----------------------- ExpressionCond ------------------------------ */

Value ExpressionCond::evaluateInternal(Variables* vars) const {
    Value pCond(vpOperand[0]->evaluateInternal(vars));
    int idx = pCond.coerceToBool() ? 1 : 2;
    return vpOperand[idx]->evaluateInternal(vars);
}

intrusive_ptr<Expression> ExpressionCond::parse(BSONElement expr, const VariablesParseState& vps) {
    if (expr.type() != Object) {
        return Base::parse(expr, vps);
    }
    verify(str::equals(expr.fieldName(), "$cond"));

    intrusive_ptr<ExpressionCond> ret = new ExpressionCond();
    ret->vpOperand.resize(3);

    const BSONObj args = expr.embeddedObject();
    BSONForEach(arg, args) {
        if (str::equals(arg.fieldName(), "if")) {
            ret->vpOperand[0] = parseOperand(arg, vps);
        } else if (str::equals(arg.fieldName(), "then")) {
            ret->vpOperand[1] = parseOperand(arg, vps);
        } else if (str::equals(arg.fieldName(), "else")) {
            ret->vpOperand[2] = parseOperand(arg, vps);
        } else {
            uasserted(17083,
                      str::stream() << "Unrecognized parameter to $cond: " << arg.fieldName());
        }
    }

    uassert(17080, "Missing 'if' parameter to $cond", ret->vpOperand[0]);
    uassert(17081, "Missing 'then' parameter to $cond", ret->vpOperand[1]);
    uassert(17082, "Missing 'else' parameter to $cond", ret->vpOperand[2]);

    return ret;
}

REGISTER_EXPRESSION("$cond", ExpressionCond::parse);
const char* ExpressionCond::getOpName() const {
    return "$cond";
}

/* ---------------------- ExpressionConstant --------------------------- */

intrusive_ptr<Expression> ExpressionConstant::parse(BSONElement exprElement,
                                                    const VariablesParseState& vps) {
    return new ExpressionConstant(Value(exprElement));
}


intrusive_ptr<ExpressionConstant> ExpressionConstant::create(const Value& pValue) {
    intrusive_ptr<ExpressionConstant> pEC(new ExpressionConstant(pValue));
    return pEC;
}

ExpressionConstant::ExpressionConstant(const Value& pTheValue) : pValue(pTheValue) {}


intrusive_ptr<Expression> ExpressionConstant::optimize() {
    /* nothing to do */
    return intrusive_ptr<Expression>(this);
}

void ExpressionConstant::addDependencies(DepsTracker* deps, vector<string>* path) const {
    /* nothing to do */
}

Value ExpressionConstant::evaluateInternal(Variables* vars) const {
    return pValue;
}

Value ExpressionConstant::serialize(bool explain) const {
    return serializeConstant(pValue);
}

REGISTER_EXPRESSION("$const", ExpressionConstant::parse);
REGISTER_EXPRESSION("$literal", ExpressionConstant::parse);  // alias
const char* ExpressionConstant::getOpName() const {
    return "$const";
}

/* ---------------------- ExpressionDateToString ----------------------- */

REGISTER_EXPRESSION("$dateToString", ExpressionDateToString::parse);
intrusive_ptr<Expression> ExpressionDateToString::parse(BSONElement expr,
                                                        const VariablesParseState& vps) {
    verify(str::equals(expr.fieldName(), "$dateToString"));

    uassert(18629, "$dateToString only supports an object as its argument", expr.type() == Object);

    BSONElement formatElem;
    BSONElement dateElem;
    const BSONObj args = expr.embeddedObject();
    BSONForEach(arg, args) {
        if (str::equals(arg.fieldName(), "format")) {
            formatElem = arg;
        } else if (str::equals(arg.fieldName(), "date")) {
            dateElem = arg;
        } else {
            uasserted(18534,
                      str::stream()
                          << "Unrecognized argument to $dateToString: " << arg.fieldName());
        }
    }

    uassert(18627, "Missing 'format' parameter to $dateToString", !formatElem.eoo());
    uassert(18628, "Missing 'date' parameter to $dateToString", !dateElem.eoo());

    uassert(18533,
            "The 'format' parameter to $dateToString must be a string literal",
            formatElem.type() == String);

    const string format = formatElem.str();

    validateFormat(format);

    return new ExpressionDateToString(format, parseOperand(dateElem, vps));
}

ExpressionDateToString::ExpressionDateToString(const string& format, intrusive_ptr<Expression> date)
    : _format(format), _date(date) {}

intrusive_ptr<Expression> ExpressionDateToString::optimize() {
    _date = _date->optimize();
    return this;
}

Value ExpressionDateToString::serialize(bool explain) const {
    return Value(
        DOC("$dateToString" << DOC("format" << _format << "date" << _date->serialize(explain))));
}

Value ExpressionDateToString::evaluateInternal(Variables* vars) const {
    const Value date = _date->evaluateInternal(vars);

    if (date.nullish()) {
        return Value(BSONNULL);
    }

    return Value(formatDate(_format, date.coerceToTm(), date.coerceToDate()));
}

// verifies that any '%' is followed by a valid format character, and that
// the format string ends with an even number of '%' symbols
void ExpressionDateToString::validateFormat(const std::string& format) {
    for (string::const_iterator it = format.begin(); it != format.end(); ++it) {
        if (*it != '%') {
            continue;
        }

        ++it;  // next character must be format modifier
        uassert(18535, "Unmatched '%' at end of $dateToString format string", it != format.end());


        switch (*it) {
            // all of these fall through intentionally
            case '%':
            case 'Y':
            case 'm':
            case 'd':
            case 'H':
            case 'M':
            case 'S':
            case 'L':
            case 'j':
            case 'w':
            case 'U':
                break;
            default:
                uasserted(18536,
                          str::stream() << "Invalid format character '%" << *it
                                        << "' in $dateToString format string");
        }
    }
}

string ExpressionDateToString::formatDate(const string& format,
                                          const tm& tm,
                                          const long long date) {
    StringBuilder formatted;
    for (string::const_iterator it = format.begin(); it != format.end(); ++it) {
        if (*it != '%') {
            formatted << *it;
            continue;
        }

        ++it;                           // next character is format modifier
        invariant(it != format.end());  // checked in validateFormat

        switch (*it) {
            case '%':  // Escaped literal %
                formatted << '%';
                break;
            case 'Y':  // Year
            {
                const int year = ExpressionYear::extract(tm);
                uassert(18537,
                        str::stream() << "$dateToString is only defined on year 0-9999,"
                                      << " tried to use year " << year,
                        (year >= 0) && (year <= 9999));
                insertPadded(formatted, year, 4);
                break;
            }
            case 'm':  // Month
                insertPadded(formatted, ExpressionMonth::extract(tm), 2);
                break;
            case 'd':  // Day of month
                insertPadded(formatted, ExpressionDayOfMonth::extract(tm), 2);
                break;
            case 'H':  // Hour
                insertPadded(formatted, ExpressionHour::extract(tm), 2);
                break;
            case 'M':  // Minute
                insertPadded(formatted, ExpressionMinute::extract(tm), 2);
                break;
            case 'S':  // Second
                insertPadded(formatted, ExpressionSecond::extract(tm), 2);
                break;
            case 'L':  // Millisecond
                insertPadded(formatted, ExpressionMillisecond::extract(date), 3);
                break;
            case 'j':  // Day of year
                insertPadded(formatted, ExpressionDayOfYear::extract(tm), 3);
                break;
            case 'w':  // Day of week
                insertPadded(formatted, ExpressionDayOfWeek::extract(tm), 1);
                break;
            case 'U':  // Week
                insertPadded(formatted, ExpressionWeek::extract(tm), 2);
                break;
            default:
                // Should never happen as format is pre-validated
                invariant(false);
        }
    }
    return formatted.str();
}

// Only works with 1 <= spaces <= 4 and 0 <= number <= 9999.
// If spaces is less than the digit count of number we simply insert the number
// without padding.
void ExpressionDateToString::insertPadded(StringBuilder& sb, int number, int width) {
    invariant(width >= 1);
    invariant(width <= 4);
    invariant(number >= 0);
    invariant(number <= 9999);

    int digits = 1;

    if (number >= 1000) {
        digits = 4;
    } else if (number >= 100) {
        digits = 3;
    } else if (number >= 10) {
        digits = 2;
    }

    if (width > digits) {
        sb.write("0000", width - digits);
    }
    sb << number;
}

void ExpressionDateToString::addDependencies(DepsTracker* deps, vector<string>* path) const {
    _date->addDependencies(deps);
}

/* ---------------------- ExpressionDayOfMonth ------------------------- */

Value ExpressionDayOfMonth::evaluateInternal(Variables* vars) const {
    Value pDate(vpOperand[0]->evaluateInternal(vars));
    return Value(extract(pDate.coerceToTm()));
}

REGISTER_EXPRESSION("$dayOfMonth", ExpressionDayOfMonth::parse);
const char* ExpressionDayOfMonth::getOpName() const {
    return "$dayOfMonth";
}

/* ------------------------- ExpressionDayOfWeek ----------------------------- */

Value ExpressionDayOfWeek::evaluateInternal(Variables* vars) const {
    Value pDate(vpOperand[0]->evaluateInternal(vars));
    return Value(extract(pDate.coerceToTm()));
}

REGISTER_EXPRESSION("$dayOfWeek", ExpressionDayOfWeek::parse);
const char* ExpressionDayOfWeek::getOpName() const {
    return "$dayOfWeek";
}

/* ------------------------- ExpressionDayOfYear ----------------------------- */

Value ExpressionDayOfYear::evaluateInternal(Variables* vars) const {
    Value pDate(vpOperand[0]->evaluateInternal(vars));
    return Value(extract(pDate.coerceToTm()));
}

REGISTER_EXPRESSION("$dayOfYear", ExpressionDayOfYear::parse);
const char* ExpressionDayOfYear::getOpName() const {
    return "$dayOfYear";
}

/* ----------------------- ExpressionDivide ---------------------------- */

Value ExpressionDivide::evaluateInternal(Variables* vars) const {
    Value lhs = vpOperand[0]->evaluateInternal(vars);
    Value rhs = vpOperand[1]->evaluateInternal(vars);

    if (lhs.numeric() && rhs.numeric()) {
        double numer = lhs.coerceToDouble();
        double denom = rhs.coerceToDouble();
        uassert(16608, "can't $divide by zero", denom != 0);

        return Value(numer / denom);
    } else if (lhs.nullish() || rhs.nullish()) {
        return Value(BSONNULL);
    } else {
        uasserted(16609,
                  str::stream() << "$divide only supports numeric types, not "
                                << typeName(lhs.getType()) << " and " << typeName(rhs.getType()));
    }
}

REGISTER_EXPRESSION("$divide", ExpressionDivide::parse);
const char* ExpressionDivide::getOpName() const {
    return "$divide";
}

/* ---------------------- ExpressionObject --------------------------- */

intrusive_ptr<ExpressionObject> ExpressionObject::create() {
    return new ExpressionObject(false);
}

intrusive_ptr<ExpressionObject> ExpressionObject::createRoot() {
    return new ExpressionObject(true);
}

ExpressionObject::ExpressionObject(bool atRoot) : _excludeId(false), _atRoot(atRoot) {}

intrusive_ptr<Expression> ExpressionObject::optimize() {
    for (FieldMap::iterator it(_expressions.begin()); it != _expressions.end(); ++it) {
        if (it->second)
            it->second = it->second->optimize();
    }

    return intrusive_ptr<Expression>(this);
}

bool ExpressionObject::isSimple() {
    for (FieldMap::iterator it(_expressions.begin()); it != _expressions.end(); ++it) {
        if (it->second && !it->second->isSimple())
            return false;
    }
    return true;
}

void ExpressionObject::addDependencies(DepsTracker* deps, vector<string>* path) const {
    string pathStr;
    if (path) {
        if (path->empty()) {
            // we are in the top level of a projection so _id is implicit
            if (!_excludeId)
                deps->fields.insert("_id");
        } else {
            FieldPath f(*path);
            pathStr = f.getPath(false);
            pathStr += '.';
        }
    } else {
        verify(!_excludeId);
    }


    for (FieldMap::const_iterator it(_expressions.begin()); it != _expressions.end(); ++it) {
        if (it->second) {
            if (path)
                path->push_back(it->first);
            it->second->addDependencies(deps, path);
            if (path)
                path->pop_back();
        } else {  // inclusion
            uassert(16407, "inclusion not supported in objects nested in $expressions", path);

            deps->fields.insert(pathStr + it->first);
        }
    }
}

void ExpressionObject::addToDocument(MutableDocument& out,
                                     const Document& currentDoc,
                                     Variables* vars) const {
    FieldMap::const_iterator end = _expressions.end();

    // This is used to mark fields we've done so that we can add the ones we haven't
    set<string> doneFields;

    FieldIterator fields(currentDoc);
    while (fields.more()) {
        Document::FieldPair field(fields.next());

        // TODO don't make a new string here
        const string fieldName = field.first.toString();
        FieldMap::const_iterator exprIter = _expressions.find(fieldName);

        // This field is not supposed to be in the output (unless it is _id)
        if (exprIter == end) {
            if (!_excludeId && _atRoot && field.first == "_id") {
                // _id from the root doc is always included (until exclusion is supported)
                // not updating doneFields since "_id" isn't in _expressions
                out.addField(field.first, field.second);
            }
            continue;
        }

        // make sure we don't add this field again
        doneFields.insert(exprIter->first);

        Expression* expr = exprIter->second.get();

        if (!expr) {
            // This means pull the matching field from the input document
            out.addField(field.first, field.second);
            continue;
        }

        ExpressionObject* exprObj = dynamic_cast<ExpressionObject*>(expr);
        BSONType valueType = field.second.getType();
        if ((valueType != Object && valueType != Array) || !exprObj) {
            // This expression replace the whole field

            Value pValue(expr->evaluateInternal(vars));

            // don't add field if nothing was found in the subobject
            if (exprObj && pValue.getDocument().empty())
                continue;

            /*
               Don't add non-existent values (note:  different from NULL or Undefined);
               this is consistent with existing selection syntax which doesn't
               force the appearance of non-existent fields.
               */
            if (!pValue.missing())
                out.addField(field.first, pValue);

            continue;
        }

        /*
            Check on the type of the input value.  If it's an
            object, just walk down into that recursively, and
            add it to the result.
        */
        if (valueType == Object) {
            MutableDocument sub(exprObj->getSizeHint());
            exprObj->addToDocument(sub, field.second.getDocument(), vars);
            out.addField(field.first, sub.freezeToValue());
        } else if (valueType == Array) {
            /*
                If it's an array, we have to do the same thing,
                but to each array element.  Then, add the array
                of results to the current document.
            */
            vector<Value> result;
            const vector<Value>& input = field.second.getArray();
            for (size_t i = 0; i < input.size(); i++) {
                // can't look for a subfield in a non-object value.
                if (input[i].getType() != Object)
                    continue;

                MutableDocument doc(exprObj->getSizeHint());
                exprObj->addToDocument(doc, input[i].getDocument(), vars);
                result.push_back(doc.freezeToValue());
            }

            out.addField(field.first, Value::consume(result));
        } else {
            verify(false);
        }
    }

    if (doneFields.size() == _expressions.size())
        return;

    /* add any remaining fields we haven't already taken care of */
    for (vector<string>::const_iterator i(_order.begin()); i != _order.end(); ++i) {
        FieldMap::const_iterator it = _expressions.find(*i);
        string fieldName(it->first);

        /* if we've already dealt with this field, above, do nothing */
        if (doneFields.count(fieldName))
            continue;

        // this is a missing inclusion field
        if (!it->second)
            continue;

        Value pValue(it->second->evaluateInternal(vars));

        /*
          Don't add non-existent values (note:  different from NULL or Undefined);
          this is consistent with existing selection syntax which doesn't
          force the appearnance of non-existent fields.
        */
        if (pValue.missing())
            continue;

        // don't add field if nothing was found in the subobject
        if (dynamic_cast<ExpressionObject*>(it->second.get()) && pValue.getDocument().empty())
            continue;


        out.addField(fieldName, pValue);
    }
}

size_t ExpressionObject::getSizeHint() const {
    // Note: this can overestimate, but that is better than underestimating
    return _expressions.size() + (_excludeId ? 0 : 1);
}

Document ExpressionObject::evaluateDocument(Variables* vars) const {
    /* create and populate the result */
    MutableDocument out(getSizeHint());

    addToDocument(out,
                  Document(),  // No inclusion field matching.
                  vars);
    return out.freeze();
}

Value ExpressionObject::evaluateInternal(Variables* vars) const {
    return Value(evaluateDocument(vars));
}

void ExpressionObject::addField(const FieldPath& fieldPath,
                                const intrusive_ptr<Expression>& pExpression) {
    const string fieldPart = fieldPath.getFieldName(0);
    const bool haveExpr = _expressions.count(fieldPart);

    intrusive_ptr<Expression>& expr = _expressions[fieldPart];  // inserts if !haveExpr
    intrusive_ptr<ExpressionObject> subObj = dynamic_cast<ExpressionObject*>(expr.get());

    if (!haveExpr) {
        _order.push_back(fieldPart);
    } else {  // we already have an expression or inclusion for this field
        if (fieldPath.getPathLength() == 1) {
            // This expression is for right here

            ExpressionObject* newSubObj = dynamic_cast<ExpressionObject*>(pExpression.get());
            uassert(16400,
                    str::stream() << "can't add an expression for field " << fieldPart
                                  << " because there is already an expression for that field"
                                  << " or one of its sub-fields.",
                    subObj && newSubObj);  // we can merge them

            // Copy everything from the newSubObj to the existing subObj
            // This is for cases like { $project:{ 'b.c':1, b:{ a:1 } } }
            for (vector<string>::const_iterator it(newSubObj->_order.begin());
                 it != newSubObj->_order.end();
                 ++it) {
                // asserts if any fields are dupes
                subObj->addField(*it, newSubObj->_expressions[*it]);
            }
            return;
        } else {
            // This expression is for a subfield
            uassert(16401,
                    str::stream() << "can't add an expression for a subfield of " << fieldPart
                                  << " because there is already an expression that applies to"
                                  << " the whole field",
                    subObj);
        }
    }

    if (fieldPath.getPathLength() == 1) {
        verify(!haveExpr);  // haveExpr case handled above.
        expr = pExpression;
        return;
    }

    if (!haveExpr)
        expr = subObj = ExpressionObject::create();

    subObj->addField(fieldPath.tail(), pExpression);
}

void ExpressionObject::includePath(const string& theFieldPath) {
    addField(theFieldPath, NULL);
}

Value ExpressionObject::serialize(bool explain) const {
    MutableDocument valBuilder;
    if (_excludeId)
        valBuilder["_id"] = Value(false);

    for (vector<string>::const_iterator it(_order.begin()); it != _order.end(); ++it) {
        string fieldName = *it;
        verify(_expressions.find(fieldName) != _expressions.end());
        intrusive_ptr<Expression> expr = _expressions.find(fieldName)->second;

        if (!expr) {
            // this is inclusion, not an expression
            valBuilder[fieldName] = Value(true);
        } else {
            valBuilder[fieldName] = expr->serialize(explain);
        }
    }
    return valBuilder.freezeToValue();
}

/* --------------------- ExpressionFieldPath --------------------------- */

// this is the old deprecated version only used by tests not using variables
intrusive_ptr<ExpressionFieldPath> ExpressionFieldPath::create(const string& fieldPath) {
    return new ExpressionFieldPath("CURRENT." + fieldPath, Variables::ROOT_ID);
}

// this is the new version that supports every syntax
intrusive_ptr<ExpressionFieldPath> ExpressionFieldPath::parse(const string& raw,
                                                              const VariablesParseState& vps) {
    uassert(16873,
            str::stream() << "FieldPath '" << raw << "' doesn't start with $",
            raw.c_str()[0] == '$');  // c_str()[0] is always a valid reference.

    uassert(16872,
            str::stream() << "'$' by itself is not a valid FieldPath",
            raw.size() >= 2);  // need at least "$" and either "$" or a field name

    if (raw[1] == '$') {
        const StringData rawSD = raw;
        const StringData fieldPath = rawSD.substr(2);  // strip off $$
        const StringData varName = fieldPath.substr(0, fieldPath.find('.'));
        Variables::uassertValidNameForUserRead(varName);
        return new ExpressionFieldPath(fieldPath.toString(), vps.getVariable(varName));
    } else {
        return new ExpressionFieldPath("CURRENT." + raw.substr(1),  // strip the "$" prefix
                                       vps.getVariable("CURRENT"));
    }
}


ExpressionFieldPath::ExpressionFieldPath(const string& theFieldPath, Variables::Id variable)
    : _fieldPath(theFieldPath), _variable(variable) {}

intrusive_ptr<Expression> ExpressionFieldPath::optimize() {
    /* nothing can be done for these */
    return intrusive_ptr<Expression>(this);
}

void ExpressionFieldPath::addDependencies(DepsTracker* deps, vector<string>* path) const {
    if (_variable == Variables::ROOT_ID) {  // includes CURRENT when it is equivalent to ROOT.
        if (_fieldPath.getPathLength() == 1) {
            deps->needWholeDocument = true;  // need full doc if just "$$ROOT"
        } else {
            deps->fields.insert(_fieldPath.tail().getPath(false));
        }
    }
}

Value ExpressionFieldPath::evaluatePathArray(size_t index, const Value& input) const {
    dassert(input.getType() == Array);

    // Check for remaining path in each element of array
    vector<Value> result;
    const vector<Value>& array = input.getArray();
    for (size_t i = 0; i < array.size(); i++) {
        if (array[i].getType() != Object)
            continue;

        const Value nested = evaluatePath(index, array[i].getDocument());
        if (!nested.missing())
            result.push_back(nested);
    }

    return Value::consume(result);
}
Value ExpressionFieldPath::evaluatePath(size_t index, const Document& input) const {
    // Note this function is very hot so it is important that is is well optimized.
    // In particular, all return paths should support RVO.

    /* if we've hit the end of the path, stop */
    if (index == _fieldPath.getPathLength() - 1)
        return input[_fieldPath.getFieldName(index)];

    // Try to dive deeper
    const Value val = input[_fieldPath.getFieldName(index)];
    switch (val.getType()) {
        case Object:
            return evaluatePath(index + 1, val.getDocument());

        case Array:
            return evaluatePathArray(index + 1, val);

        default:
            return Value();
    }
}

Value ExpressionFieldPath::evaluateInternal(Variables* vars) const {
    if (_fieldPath.getPathLength() == 1)  // get the whole variable
        return vars->getValue(_variable);

    if (_variable == Variables::ROOT_ID) {
        // ROOT is always a document so use optimized code path
        return evaluatePath(1, vars->getRoot());
    }

    Value var = vars->getValue(_variable);
    switch (var.getType()) {
        case Object:
            return evaluatePath(1, var.getDocument());
        case Array:
            return evaluatePathArray(1, var);
        default:
            return Value();
    }
}

Value ExpressionFieldPath::serialize(bool explain) const {
    if (_fieldPath.getFieldName(0) == "CURRENT" && _fieldPath.getPathLength() > 1) {
        // use short form for "$$CURRENT.foo" but not just "$$CURRENT"
        return Value("$" + _fieldPath.tail().getPath(false));
    } else {
        return Value("$$" + _fieldPath.getPath(false));
    }
}

/* ------------------------- ExpressionLet ----------------------------- */

REGISTER_EXPRESSION("$let", ExpressionLet::parse);
intrusive_ptr<Expression> ExpressionLet::parse(BSONElement expr, const VariablesParseState& vpsIn) {
    verify(str::equals(expr.fieldName(), "$let"));

    uassert(16874, "$let only supports an object as its argument", expr.type() == Object);
    const BSONObj args = expr.embeddedObject();

    // varsElem must be parsed before inElem regardless of BSON order.
    BSONElement varsElem;
    BSONElement inElem;
    BSONForEach(arg, args) {
        if (str::equals(arg.fieldName(), "vars")) {
            varsElem = arg;
        } else if (str::equals(arg.fieldName(), "in")) {
            inElem = arg;
        } else {
            uasserted(16875,
                      str::stream() << "Unrecognized parameter to $let: " << arg.fieldName());
        }
    }

    uassert(16876, "Missing 'vars' parameter to $let", !varsElem.eoo());
    uassert(16877, "Missing 'in' parameter to $let", !inElem.eoo());

    // parse "vars"
    VariablesParseState vpsSub(vpsIn);  // vpsSub gets our vars, vpsIn doesn't.
    VariableMap vars;
    BSONForEach(varElem, varsElem.embeddedObjectUserCheck()) {
        const string varName = varElem.fieldName();
        Variables::uassertValidNameForUserWrite(varName);
        Variables::Id id = vpsSub.defineVariable(varName);

        vars[id] = NameAndExpression(varName, parseOperand(varElem, vpsIn));  // only has outer vars
    }

    // parse "in"
    intrusive_ptr<Expression> subExpression = parseOperand(inElem, vpsSub);  // has our vars

    return new ExpressionLet(vars, subExpression);
}

ExpressionLet::ExpressionLet(const VariableMap& vars, intrusive_ptr<Expression> subExpression)
    : _variables(vars), _subExpression(subExpression) {}

intrusive_ptr<Expression> ExpressionLet::optimize() {
    if (_variables.empty()) {
        // we aren't binding any variables so just return the subexpression
        return _subExpression->optimize();
    }

    for (VariableMap::iterator it = _variables.begin(), end = _variables.end(); it != end; ++it) {
        it->second.expression = it->second.expression->optimize();
    }

    // TODO be smarter with constant "variables"
    _subExpression = _subExpression->optimize();

    return this;
}

Value ExpressionLet::serialize(bool explain) const {
    MutableDocument vars;
    for (VariableMap::const_iterator it = _variables.begin(), end = _variables.end(); it != end;
         ++it) {
        vars[it->second.name] = it->second.expression->serialize(explain);
    }

    return Value(
        DOC("$let" << DOC("vars" << vars.freeze() << "in" << _subExpression->serialize(explain))));
}

Value ExpressionLet::evaluateInternal(Variables* vars) const {
    for (VariableMap::const_iterator it = _variables.begin(), end = _variables.end(); it != end;
         ++it) {
        // It is guaranteed at parse-time that these expressions don't use the variable ids we
        // are setting
        vars->setValue(it->first, it->second.expression->evaluateInternal(vars));
    }

    return _subExpression->evaluateInternal(vars);
}

void ExpressionLet::addDependencies(DepsTracker* deps, vector<string>* path) const {
    for (VariableMap::const_iterator it = _variables.begin(), end = _variables.end(); it != end;
         ++it) {
        it->second.expression->addDependencies(deps);
    }

    // TODO be smarter when CURRENT is a bound variable
    _subExpression->addDependencies(deps);
}


/* ------------------------- ExpressionMap ----------------------------- */

REGISTER_EXPRESSION("$map", ExpressionMap::parse);
intrusive_ptr<Expression> ExpressionMap::parse(BSONElement expr, const VariablesParseState& vpsIn) {
    verify(str::equals(expr.fieldName(), "$map"));

    uassert(16878, "$map only supports an object as it's argument", expr.type() == Object);

    // "in" must be parsed after "as" regardless of BSON order
    BSONElement inputElem;
    BSONElement asElem;
    BSONElement inElem;
    const BSONObj args = expr.embeddedObject();
    BSONForEach(arg, args) {
        if (str::equals(arg.fieldName(), "input")) {
            inputElem = arg;
        } else if (str::equals(arg.fieldName(), "as")) {
            asElem = arg;
        } else if (str::equals(arg.fieldName(), "in")) {
            inElem = arg;
        } else {
            uasserted(16879,
                      str::stream() << "Unrecognized parameter to $map: " << arg.fieldName());
        }
    }

    uassert(16880, "Missing 'input' parameter to $map", !inputElem.eoo());
    uassert(16881, "Missing 'as' parameter to $map", !asElem.eoo());
    uassert(16882, "Missing 'in' parameter to $map", !inElem.eoo());

    // parse "input"
    intrusive_ptr<Expression> input = parseOperand(inputElem, vpsIn);  // only has outer vars

    // parse "as"
    VariablesParseState vpsSub(vpsIn);  // vpsSub gets our vars, vpsIn doesn't.
    string varName = asElem.str();
    Variables::uassertValidNameForUserWrite(varName);
    Variables::Id varId = vpsSub.defineVariable(varName);

    // parse "in"
    intrusive_ptr<Expression> in = parseOperand(inElem, vpsSub);  // has access to map variable

    return new ExpressionMap(varName, varId, input, in);
}

ExpressionMap::ExpressionMap(const string& varName,
                             Variables::Id varId,
                             intrusive_ptr<Expression> input,
                             intrusive_ptr<Expression> each)
    : _varName(varName), _varId(varId), _input(input), _each(each) {}

intrusive_ptr<Expression> ExpressionMap::optimize() {
    // TODO handle when _input is constant
    _input = _input->optimize();
    _each = _each->optimize();
    return this;
}

Value ExpressionMap::serialize(bool explain) const {
    return Value(DOC("$map" << DOC("input" << _input->serialize(explain) << "as" << _varName << "in"
                                           << _each->serialize(explain))));
}

Value ExpressionMap::evaluateInternal(Variables* vars) const {
    // guaranteed at parse time that this isn't using our _varId
    const Value inputVal = _input->evaluateInternal(vars);
    if (inputVal.nullish())
        return Value(BSONNULL);

    uassert(16883,
            str::stream() << "input to $map must be an Array not " << typeName(inputVal.getType()),
            inputVal.getType() == Array);

    const vector<Value>& input = inputVal.getArray();

    if (input.empty())
        return inputVal;

    vector<Value> output;
    output.reserve(input.size());
    for (size_t i = 0; i < input.size(); i++) {
        vars->setValue(_varId, input[i]);

        Value toInsert = _each->evaluateInternal(vars);
        if (toInsert.missing())
            toInsert = Value(BSONNULL);  // can't insert missing values into array

        output.push_back(toInsert);
    }

    return Value::consume(output);
}

void ExpressionMap::addDependencies(DepsTracker* deps, vector<string>* path) const {
    _input->addDependencies(deps);
    _each->addDependencies(deps);
}

/* ------------------------- ExpressionMeta ----------------------------- */

REGISTER_EXPRESSION("$meta", ExpressionMeta::parse);
intrusive_ptr<Expression> ExpressionMeta::parse(BSONElement expr,
                                                const VariablesParseState& vpsIn) {
    uassert(17307, "$meta only supports String arguments", expr.type() == String);
    uassert(17308, "Unsupported argument to $meta: " + expr.String(), expr.String() == "textScore");

    return new ExpressionMeta();
}

Value ExpressionMeta::serialize(bool explain) const {
    return Value(DOC("$meta"
                     << "textScore"));
}

Value ExpressionMeta::evaluateInternal(Variables* vars) const {
    const Document& root = vars->getRoot();
    return root.hasTextScore() ? Value(root.getTextScore()) : Value();
}

void ExpressionMeta::addDependencies(DepsTracker* deps, vector<string>* path) const {
    deps->needTextScore = true;
}

/* ------------------------- ExpressionMillisecond ----------------------------- */

Value ExpressionMillisecond::evaluateInternal(Variables* vars) const {
    Value date(vpOperand[0]->evaluateInternal(vars));
    return Value(extract(date.coerceToDate()));
}

int ExpressionMillisecond::extract(const long long date) {
    const int ms = date % 1000LL;
    // adding 1000 since dates before 1970 would have negative ms
    return ms >= 0 ? ms : 1000 + ms;
}

REGISTER_EXPRESSION("$millisecond", ExpressionMillisecond::parse);
const char* ExpressionMillisecond::getOpName() const {
    return "$millisecond";
}

/* ------------------------- ExpressionMinute -------------------------- */

Value ExpressionMinute::evaluateInternal(Variables* vars) const {
    Value pDate(vpOperand[0]->evaluateInternal(vars));
    return Value(extract(pDate.coerceToTm()));
}

REGISTER_EXPRESSION("$minute", ExpressionMinute::parse);
const char* ExpressionMinute::getOpName() const {
    return "$minute";
}

/* ----------------------- ExpressionMod ---------------------------- */

Value ExpressionMod::evaluateInternal(Variables* vars) const {
    Value lhs = vpOperand[0]->evaluateInternal(vars);
    Value rhs = vpOperand[1]->evaluateInternal(vars);

    BSONType leftType = lhs.getType();
    BSONType rightType = rhs.getType();

    if (lhs.numeric() && rhs.numeric()) {
        // ensure we aren't modding by 0
        double right = rhs.coerceToDouble();

        uassert(16610, "can't $mod by 0", right != 0);

        if (leftType == NumberDouble || (rightType == NumberDouble && rhs.coerceToInt() != right)) {
            // the shell converts ints to doubles so if right is larger than int max or
            // if right truncates to something other than itself, it is a real double.
            // Integer-valued double case is handled below

            double left = lhs.coerceToDouble();
            return Value(fmod(left, right));
        } else if (leftType == NumberLong || rightType == NumberLong) {
            // if either is long, return long
            long long left = lhs.coerceToLong();
            long long rightLong = rhs.coerceToLong();
            return Value(left % rightLong);
        }

        // lastly they must both be ints, return int
        int left = lhs.coerceToInt();
        int rightInt = rhs.coerceToInt();
        return Value(left % rightInt);
    } else if (lhs.nullish() || rhs.nullish()) {
        return Value(BSONNULL);
    } else {
        uasserted(16611,
                  str::stream() << "$mod only supports numeric types, not "
                                << typeName(lhs.getType()) << " and " << typeName(rhs.getType()));
    }
}

REGISTER_EXPRESSION("$mod", ExpressionMod::parse);
const char* ExpressionMod::getOpName() const {
    return "$mod";
}

/* ------------------------ ExpressionMonth ----------------------------- */

Value ExpressionMonth::evaluateInternal(Variables* vars) const {
    Value pDate(vpOperand[0]->evaluateInternal(vars));
    return Value(extract(pDate.coerceToTm()));
}

REGISTER_EXPRESSION("$month", ExpressionMonth::parse);
const char* ExpressionMonth::getOpName() const {
    return "$month";
}

/* ------------------------- ExpressionMultiply ----------------------------- */

Value ExpressionMultiply::evaluateInternal(Variables* vars) const {
    /*
      We'll try to return the narrowest possible result value.  To do that
      without creating intermediate Values, do the arithmetic for double
      and integral types in parallel, tracking the current narrowest
      type.
     */
    double doubleProduct = 1;
    long long longProduct = 1;
    BSONType productType = NumberInt;

    const size_t n = vpOperand.size();
    for (size_t i = 0; i < n; ++i) {
        Value val = vpOperand[i]->evaluateInternal(vars);

        if (val.numeric()) {
            productType = Value::getWidestNumeric(productType, val.getType());

            doubleProduct *= val.coerceToDouble();
            longProduct *= val.coerceToLong();
        } else if (val.nullish()) {
            return Value(BSONNULL);
        } else {
            uasserted(16555,
                      str::stream() << "$multiply only supports numeric types, not "
                                    << typeName(val.getType()));
        }
    }

    if (productType == NumberDouble)
        return Value(doubleProduct);
    else if (productType == NumberLong)
        return Value(longProduct);
    else if (productType == NumberInt)
        return Value::createIntOrLong(longProduct);
    else
        massert(16418, "$multiply resulted in a non-numeric type", false);
}

REGISTER_EXPRESSION("$multiply", ExpressionMultiply::parse);
const char* ExpressionMultiply::getOpName() const {
    return "$multiply";
}

/* ------------------------- ExpressionHour ----------------------------- */

Value ExpressionHour::evaluateInternal(Variables* vars) const {
    Value pDate(vpOperand[0]->evaluateInternal(vars));
    return Value(extract(pDate.coerceToTm()));
}

REGISTER_EXPRESSION("$hour", ExpressionHour::parse);
const char* ExpressionHour::getOpName() const {
    return "$hour";
}

/* ----------------------- ExpressionIfNull ---------------------------- */

Value ExpressionIfNull::evaluateInternal(Variables* vars) const {
    Value pLeft(vpOperand[0]->evaluateInternal(vars));
    if (!pLeft.nullish())
        return pLeft;

    Value pRight(vpOperand[1]->evaluateInternal(vars));
    return pRight;
}

REGISTER_EXPRESSION("$ifNull", ExpressionIfNull::parse);
const char* ExpressionIfNull::getOpName() const {
    return "$ifNull";
}

/* ------------------------ ExpressionNary ----------------------------- */

intrusive_ptr<Expression> ExpressionNary::optimize() {
    const size_t n = vpOperand.size();

    // optimize sub-expressions and count constants
    unsigned constCount = 0;
    for (size_t i = 0; i < n; ++i) {
        intrusive_ptr<Expression> optimized = vpOperand[i]->optimize();

        // substitute the optimized expression
        vpOperand[i] = optimized;

        // check to see if the result was a constant
        if (dynamic_cast<ExpressionConstant*>(optimized.get())) {
            constCount++;
        }
    }

    // If all the operands are constant, we can replace this expression with a constant. Using
    // an empty Variables since it will never be accessed.
    if (constCount == n) {
        Variables emptyVars;
        Value pResult(evaluateInternal(&emptyVars));
        intrusive_ptr<Expression> pReplacement(ExpressionConstant::create(pResult));
        return pReplacement;
    }

    // Remaining optimizations are only for associative and commutative expressions.
    if (!isAssociativeAndCommutative())
        return this;

    // Process vpOperand to split it into constant and nonconstant vectors.
    // This can leave vpOperand in an invalid state that is cleaned up after the loop.
    ExpressionVector constExprs;
    ExpressionVector nonConstExprs;
    for (size_t i = 0; i < vpOperand.size(); ++i) {  // NOTE: vpOperand grows in loop
        intrusive_ptr<Expression> expr = vpOperand[i];
        if (dynamic_cast<ExpressionConstant*>(expr.get())) {
            constExprs.push_back(expr);
        } else {
            // If the child operand is the same type as this, then we can
            // extract its operands and inline them here because we know
            // this is commutative and associative.  We detect sameness of
            // the child operator by checking for equality of the opNames
            ExpressionNary* nary = dynamic_cast<ExpressionNary*>(expr.get());
            if (!nary || !str::equals(nary->getOpName(), getOpName())) {
                nonConstExprs.push_back(expr);
            } else {
                // same expression, so flatten by adding to vpOperand which
                // will be processed later in this loop.
                vpOperand.insert(vpOperand.end(), nary->vpOperand.begin(), nary->vpOperand.end());
            }
        }
    }

    // collapse all constant expressions (if any)
    Value constValue;
    if (!constExprs.empty()) {
        vpOperand = constExprs;
        Variables emptyVars;
        constValue = evaluateInternal(&emptyVars);
    }

    // now set the final expression list with constant (if any) at the end
    vpOperand = nonConstExprs;
    if (!constExprs.empty()) {
        vpOperand.push_back(ExpressionConstant::create(constValue));
    }

    return this;
}

void ExpressionNary::addDependencies(DepsTracker* deps, vector<string>* path) const {
    for (ExpressionVector::const_iterator i(vpOperand.begin()); i != vpOperand.end(); ++i) {
        (*i)->addDependencies(deps);
    }
}

void ExpressionNary::addOperand(const intrusive_ptr<Expression>& pExpression) {
    vpOperand.push_back(pExpression);
}

Value ExpressionNary::serialize(bool explain) const {
    const size_t nOperand = vpOperand.size();
    vector<Value> array;
    /* build up the array */
    for (size_t i = 0; i < nOperand; i++)
        array.push_back(vpOperand[i]->serialize(explain));

    return Value(DOC(getOpName() << array));
}

/* ------------------------- ExpressionNot ----------------------------- */

Value ExpressionNot::evaluateInternal(Variables* vars) const {
    Value pOp(vpOperand[0]->evaluateInternal(vars));

    bool b = pOp.coerceToBool();
    return Value(!b);
}

REGISTER_EXPRESSION("$not", ExpressionNot::parse);
const char* ExpressionNot::getOpName() const {
    return "$not";
}

/* -------------------------- ExpressionOr ----------------------------- */

Value ExpressionOr::evaluateInternal(Variables* vars) const {
    const size_t n = vpOperand.size();
    for (size_t i = 0; i < n; ++i) {
        Value pValue(vpOperand[i]->evaluateInternal(vars));
        if (pValue.coerceToBool())
            return Value(true);
    }

    return Value(false);
}

intrusive_ptr<Expression> ExpressionOr::optimize() {
    /* optimize the disjunction as much as possible */
    intrusive_ptr<Expression> pE(ExpressionNary::optimize());

    /* if the result isn't a disjunction, we can't do anything */
    ExpressionOr* pOr = dynamic_cast<ExpressionOr*>(pE.get());
    if (!pOr)
        return pE;

    /*
      Check the last argument on the result; if it's not constant (as
      promised by ExpressionNary::optimize(),) then there's nothing
      we can do.
    */
    const size_t n = pOr->vpOperand.size();
    // ExpressionNary::optimize() generates an ExpressionConstant for {$or:[]}.
    verify(n > 0);
    intrusive_ptr<Expression> pLast(pOr->vpOperand[n - 1]);
    const ExpressionConstant* pConst = dynamic_cast<ExpressionConstant*>(pLast.get());
    if (!pConst)
        return pE;

    /*
      Evaluate and coerce the last argument to a boolean.  If it's true,
      then we can replace this entire expression.
     */
    bool last = pConst->getValue().coerceToBool();
    if (last) {
        intrusive_ptr<ExpressionConstant> pFinal(ExpressionConstant::create(Value(true)));
        return pFinal;
    }

    /*
      If we got here, the final operand was false, so we don't need it
      anymore.  If there was only one other operand, we don't need the
      conjunction either.  Note we still need to keep the promise that
      the result will be a boolean.
     */
    if (n == 2) {
        intrusive_ptr<Expression> pFinal(ExpressionCoerceToBool::create(pOr->vpOperand[0]));
        return pFinal;
    }

    /*
      Remove the final "false" value, and return the new expression.
    */
    pOr->vpOperand.resize(n - 1);
    return pE;
}

REGISTER_EXPRESSION("$or", ExpressionOr::parse);
const char* ExpressionOr::getOpName() const {
    return "$or";
}

/* ------------------------- ExpressionSecond ----------------------------- */

Value ExpressionSecond::evaluateInternal(Variables* vars) const {
    Value pDate(vpOperand[0]->evaluateInternal(vars));
    return Value(extract(pDate.coerceToTm()));
}

REGISTER_EXPRESSION("$second", ExpressionSecond::parse);
const char* ExpressionSecond::getOpName() const {
    return "$second";
}

namespace {
ValueSet arrayToSet(const Value& val) {
    const vector<Value>& array = val.getArray();
    return ValueSet(array.begin(), array.end());
}
}

/* ----------------------- ExpressionSetDifference ---------------------------- */

Value ExpressionSetDifference::evaluateInternal(Variables* vars) const {
    const Value lhs = vpOperand[0]->evaluateInternal(vars);
    const Value rhs = vpOperand[1]->evaluateInternal(vars);

    if (lhs.nullish() || rhs.nullish()) {
        return Value(BSONNULL);
    }

    uassert(17048,
            str::stream() << "both operands of $setDifference must be arrays. First "
                          << "argument is of type: " << typeName(lhs.getType()),
            lhs.getType() == Array);
    uassert(17049,
            str::stream() << "both operands of $setDifference must be arrays. Second "
                          << "argument is of type: " << typeName(rhs.getType()),
            rhs.getType() == Array);

    ValueSet rhsSet = arrayToSet(rhs);
    const vector<Value>& lhsArray = lhs.getArray();
    vector<Value> returnVec;

    for (vector<Value>::const_iterator it = lhsArray.begin(); it != lhsArray.end(); ++it) {
        // rhsSet serves the dual role of filtering out elements that were originally present
        // in RHS and of eleminating duplicates from LHS
        if (rhsSet.insert(*it).second) {
            returnVec.push_back(*it);
        }
    }
    return Value::consume(returnVec);
}

REGISTER_EXPRESSION("$setDifference", ExpressionSetDifference::parse);
const char* ExpressionSetDifference::getOpName() const {
    return "$setDifference";
}

/* ----------------------- ExpressionSetEquals ---------------------------- */

void ExpressionSetEquals::validateArguments(const ExpressionVector& args) const {
    uassert(17045,
            str::stream() << "$setEquals needs at least two arguments had: " << args.size(),
            args.size() >= 2);
}

Value ExpressionSetEquals::evaluateInternal(Variables* vars) const {
    const size_t n = vpOperand.size();
    std::set<Value> lhs;

    for (size_t i = 0; i < n; i++) {
        const Value nextEntry = vpOperand[i]->evaluateInternal(vars);
        uassert(17044,
                str::stream() << "All operands of $setEquals must be arrays. One "
                              << "argument is of type: " << typeName(nextEntry.getType()),
                nextEntry.getType() == Array);

        if (i == 0) {
            lhs.insert(nextEntry.getArray().begin(), nextEntry.getArray().end());
        } else {
            const std::set<Value> rhs(nextEntry.getArray().begin(), nextEntry.getArray().end());
            if (lhs != rhs) {
                return Value(false);
            }
        }
    }
    return Value(true);
}

REGISTER_EXPRESSION("$setEquals", ExpressionSetEquals::parse);
const char* ExpressionSetEquals::getOpName() const {
    return "$setEquals";
}

/* ----------------------- ExpressionSetIntersection ---------------------------- */

Value ExpressionSetIntersection::evaluateInternal(Variables* vars) const {
    const size_t n = vpOperand.size();
    ValueSet currentIntersection;
    for (size_t i = 0; i < n; i++) {
        const Value nextEntry = vpOperand[i]->evaluateInternal(vars);
        if (nextEntry.nullish()) {
            return Value(BSONNULL);
        }
        uassert(17047,
                str::stream() << "All operands of $setIntersection must be arrays. One "
                              << "argument is of type: " << typeName(nextEntry.getType()),
                nextEntry.getType() == Array);

        if (i == 0) {
            currentIntersection.insert(nextEntry.getArray().begin(), nextEntry.getArray().end());
        } else {
            ValueSet nextSet = arrayToSet(nextEntry);
            if (currentIntersection.size() > nextSet.size()) {
                // to iterate over whichever is the smaller set
                nextSet.swap(currentIntersection);
            }
            ValueSet::iterator it = currentIntersection.begin();
            while (it != currentIntersection.end()) {
                if (!nextSet.count(*it)) {
                    ValueSet::iterator del = it;
                    ++it;
                    currentIntersection.erase(del);
                } else {
                    ++it;
                }
            }
        }
        if (currentIntersection.empty()) {
            break;
        }
    }
    vector<Value> result = vector<Value>(currentIntersection.begin(), currentIntersection.end());
    return Value::consume(result);
}

REGISTER_EXPRESSION("$setIntersection", ExpressionSetIntersection::parse);
const char* ExpressionSetIntersection::getOpName() const {
    return "$setIntersection";
}

/* ----------------------- ExpressionSetIsSubset ---------------------------- */

namespace {
Value setIsSubsetHelper(const vector<Value>& lhs, const ValueSet& rhs) {
    // do not shortcircuit when lhs.size() > rhs.size()
    // because lhs can have redundant entries
    for (vector<Value>::const_iterator it = lhs.begin(); it != lhs.end(); ++it) {
        if (!rhs.count(*it)) {
            return Value(false);
        }
    }
    return Value(true);
}
}

Value ExpressionSetIsSubset::evaluateInternal(Variables* vars) const {
    const Value lhs = vpOperand[0]->evaluateInternal(vars);
    const Value rhs = vpOperand[1]->evaluateInternal(vars);

    uassert(17046,
            str::stream() << "both operands of $setIsSubset must be arrays. First "
                          << "argument is of type: " << typeName(lhs.getType()),
            lhs.getType() == Array);
    uassert(17042,
            str::stream() << "both operands of $setIsSubset must be arrays. Second "
                          << "argument is of type: " << typeName(rhs.getType()),
            rhs.getType() == Array);

    return setIsSubsetHelper(lhs.getArray(), arrayToSet(rhs));
}

/**
 * This class handles the case where the RHS set is constant.
 *
 * Since it is constant we can construct the hashset once which makes the runtime performance
 * effectively constant with respect to the size of RHS. Large, constant RHS is expected to be a
 * major use case for $redact and this has been verified to improve performance significantly.
 */
class ExpressionSetIsSubset::Optimized : public ExpressionSetIsSubset {
public:
    Optimized(const ValueSet& cachedRhsSet, const ExpressionVector& operands)
        : _cachedRhsSet(cachedRhsSet) {
        vpOperand = operands;
    }

    virtual Value evaluateInternal(Variables* vars) const {
        const Value lhs = vpOperand[0]->evaluateInternal(vars);

        uassert(17310,
                str::stream() << "both operands of $setIsSubset must be arrays. First "
                              << "argument is of type: " << typeName(lhs.getType()),
                lhs.getType() == Array);

        return setIsSubsetHelper(lhs.getArray(), _cachedRhsSet);
    }

private:
    const ValueSet _cachedRhsSet;
};

intrusive_ptr<Expression> ExpressionSetIsSubset::optimize() {
    // perfore basic optimizations
    intrusive_ptr<Expression> optimized = ExpressionNary::optimize();

    // if ExpressionNary::optimize() created a new value, return it directly
    if (optimized.get() != this)
        return optimized;

    if (ExpressionConstant* ec = dynamic_cast<ExpressionConstant*>(vpOperand[1].get())) {
        const Value rhs = ec->getValue();
        uassert(17311,
                str::stream() << "both operands of $setIsSubset must be arrays. Second "
                              << "argument is of type: " << typeName(rhs.getType()),
                rhs.getType() == Array);

        return new Optimized(arrayToSet(rhs), vpOperand);
    }

    return optimized;
}

REGISTER_EXPRESSION("$setIsSubset", ExpressionSetIsSubset::parse);
const char* ExpressionSetIsSubset::getOpName() const {
    return "$setIsSubset";
}

/* ----------------------- ExpressionSetUnion ---------------------------- */

Value ExpressionSetUnion::evaluateInternal(Variables* vars) const {
    ValueSet unionedSet;
    const size_t n = vpOperand.size();
    for (size_t i = 0; i < n; i++) {
        const Value newEntries = vpOperand[i]->evaluateInternal(vars);
        if (newEntries.nullish()) {
            return Value(BSONNULL);
        }
        uassert(17043,
                str::stream() << "All operands of $setUnion must be arrays. One argument"
                              << " is of type: " << typeName(newEntries.getType()),
                newEntries.getType() == Array);

        unionedSet.insert(newEntries.getArray().begin(), newEntries.getArray().end());
    }
    vector<Value> result = vector<Value>(unionedSet.begin(), unionedSet.end());
    return Value::consume(result);
}

REGISTER_EXPRESSION("$setUnion", ExpressionSetUnion::parse);
const char* ExpressionSetUnion::getOpName() const {
    return "$setUnion";
}

/* ----------------------- ExpressionSize ---------------------------- */

Value ExpressionSize::evaluateInternal(Variables* vars) const {
    Value array = vpOperand[0]->evaluateInternal(vars);

    uassert(17124,
            str::stream() << "The argument to $size must be an Array, but was of type: "
                          << typeName(array.getType()),
            array.getType() == Array);
    return Value::createIntOrLong(array.getArray().size());
}

REGISTER_EXPRESSION("$size", ExpressionSize::parse);
const char* ExpressionSize::getOpName() const {
    return "$size";
}

/* ----------------------- ExpressionStrcasecmp ---------------------------- */

Value ExpressionStrcasecmp::evaluateInternal(Variables* vars) const {
    Value pString1(vpOperand[0]->evaluateInternal(vars));
    Value pString2(vpOperand[1]->evaluateInternal(vars));

    /* boost::iequals returns a bool not an int so strings must actually be allocated */
    string str1 = boost::to_upper_copy(pString1.coerceToString());
    string str2 = boost::to_upper_copy(pString2.coerceToString());
    int result = str1.compare(str2);

    if (result == 0)
        return Value(0);
    else if (result > 0)
        return Value(1);
    else
        return Value(-1);
}

REGISTER_EXPRESSION("$strcasecmp", ExpressionStrcasecmp::parse);
const char* ExpressionStrcasecmp::getOpName() const {
    return "$strcasecmp";
}

/* ----------------------- ExpressionSubstr ---------------------------- */

Value ExpressionSubstr::evaluateInternal(Variables* vars) const {
    Value pString(vpOperand[0]->evaluateInternal(vars));
    Value pLower(vpOperand[1]->evaluateInternal(vars));
    Value pLength(vpOperand[2]->evaluateInternal(vars));

    string str = pString.coerceToString();
    uassert(16034,
            str::stream() << getOpName()
                          << ":  starting index must be a numeric type (is BSON type "
                          << typeName(pLower.getType()) << ")",
            (pLower.getType() == NumberInt || pLower.getType() == NumberLong ||
             pLower.getType() == NumberDouble));
    uassert(16035,
            str::stream() << getOpName() << ":  length must be a numeric type (is BSON type "
                          << typeName(pLength.getType()) << ")",
            (pLength.getType() == NumberInt || pLength.getType() == NumberLong ||
             pLength.getType() == NumberDouble));
    string::size_type lower = static_cast<string::size_type>(pLower.coerceToLong());
    string::size_type length = static_cast<string::size_type>(pLength.coerceToLong());
    if (lower >= str.length()) {
        // If lower > str.length() then string::substr() will throw out_of_range, so return an
        // empty string if lower is not a valid string index.
        return Value("");
    }
    return Value(str.substr(lower, length));
}

REGISTER_EXPRESSION("$substr", ExpressionSubstr::parse);
const char* ExpressionSubstr::getOpName() const {
    return "$substr";
}

/* ----------------------- ExpressionSubtract ---------------------------- */

Value ExpressionSubtract::evaluateInternal(Variables* vars) const {
    Value lhs = vpOperand[0]->evaluateInternal(vars);
    Value rhs = vpOperand[1]->evaluateInternal(vars);

    BSONType diffType = Value::getWidestNumeric(rhs.getType(), lhs.getType());

    if (diffType == NumberDouble) {
        double right = rhs.coerceToDouble();
        double left = lhs.coerceToDouble();
        return Value(left - right);
    } else if (diffType == NumberLong) {
        long long right = rhs.coerceToLong();
        long long left = lhs.coerceToLong();
        return Value(left - right);
    } else if (diffType == NumberInt) {
        long long right = rhs.coerceToLong();
        long long left = lhs.coerceToLong();
        return Value::createIntOrLong(left - right);
    } else if (lhs.nullish() || rhs.nullish()) {
        return Value(BSONNULL);
    } else if (lhs.getType() == Date) {
        if (rhs.getType() == Date) {
            long long timeDelta = lhs.getDate() - rhs.getDate();
            return Value(timeDelta);
        } else if (rhs.numeric()) {
            long long millisSinceEpoch = lhs.getDate() - rhs.coerceToLong();
            return Value(Date_t(millisSinceEpoch));
        } else {
            uasserted(16613,
                      str::stream() << "cant $subtract a " << typeName(rhs.getType())
                                    << " from a Date");
        }
    } else {
        uasserted(16556,
                  str::stream() << "cant $subtract a" << typeName(rhs.getType()) << " from a "
                                << typeName(lhs.getType()));
    }
}

REGISTER_EXPRESSION("$subtract", ExpressionSubtract::parse);
const char* ExpressionSubtract::getOpName() const {
    return "$subtract";
}

/* ------------------------- ExpressionToLower ----------------------------- */

Value ExpressionToLower::evaluateInternal(Variables* vars) const {
    Value pString(vpOperand[0]->evaluateInternal(vars));
    string str = pString.coerceToString();
    boost::to_lower(str);
    return Value(str);
}

REGISTER_EXPRESSION("$toLower", ExpressionToLower::parse);
const char* ExpressionToLower::getOpName() const {
    return "$toLower";
}

/* ------------------------- ExpressionToUpper -------------------------- */

Value ExpressionToUpper::evaluateInternal(Variables* vars) const {
    Value pString(vpOperand[0]->evaluateInternal(vars));
    string str(pString.coerceToString());
    boost::to_upper(str);
    return Value(str);
}

REGISTER_EXPRESSION("$toUpper", ExpressionToUpper::parse);
const char* ExpressionToUpper::getOpName() const {
    return "$toUpper";
}

/* ------------------------- ExpressionWeek ----------------------------- */

Value ExpressionWeek::evaluateInternal(Variables* vars) const {
    Value pDate(vpOperand[0]->evaluateInternal(vars));
    return Value(extract(pDate.coerceToTm()));
}

int ExpressionWeek::extract(const tm& tm) {
    int dayOfWeek = tm.tm_wday;
    int dayOfYear = tm.tm_yday;
    int prevSundayDayOfYear = dayOfYear - dayOfWeek;    // may be negative
    int nextSundayDayOfYear = prevSundayDayOfYear + 7;  // must be positive

    // Return the zero based index of the week of the next sunday, equal to the one based index
    // of the week of the previous sunday, which is to be returned.
    int nextSundayWeek = nextSundayDayOfYear / 7;

    // Verify that the week calculation is consistent with strftime "%U".
    DEV {
        char buf[3];
        verify(strftime(buf, 3, "%U", &tm));
        verify(int(str::toUnsigned(buf)) == nextSundayWeek);
    }

    return nextSundayWeek;
}

REGISTER_EXPRESSION("$week", ExpressionWeek::parse);
const char* ExpressionWeek::getOpName() const {
    return "$week";
}

/* ------------------------- ExpressionYear ----------------------------- */

Value ExpressionYear::evaluateInternal(Variables* vars) const {
    Value pDate(vpOperand[0]->evaluateInternal(vars));
    return Value(extract(pDate.coerceToTm()));
}

REGISTER_EXPRESSION("$year", ExpressionYear::parse);
const char* ExpressionYear::getOpName() const {
    return "$year";
}
}
