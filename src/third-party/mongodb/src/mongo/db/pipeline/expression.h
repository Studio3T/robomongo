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

#pragma once

#include "mongo/platform/basic.h"

#include <boost/intrusive_ptr.hpp>

#include "mongo/db/pipeline/dependencies.h"
#include "mongo/db/pipeline/document.h"
#include "mongo/db/pipeline/field_path.h"
#include "mongo/db/pipeline/value.h"
#include "mongo/util/intrusive_counter.h"
#include "mongo/util/mongoutils/str.h"
#include "mongo/util/string_map.h"

namespace mongo {

class BSONArrayBuilder;
class BSONElement;
class BSONObjBuilder;
class DocumentSource;

// TODO: Look into merging with ExpressionContext and possibly ObjectCtx.
/// The state used as input and working space for Expressions.
class Variables {
    MONGO_DISALLOW_COPYING(Variables);

public:
    /**
     * Each unique variable is assigned a unique id of this type
     */
    typedef size_t Id;

    // This is only for expressions that use no variables (even ROOT).
    Variables() : _numVars(0) {}

    explicit Variables(size_t numVars, const Document& root = Document())
        : _root(root), _rest(numVars == 0 ? NULL : new Value[numVars]), _numVars(numVars) {}

    static void uassertValidNameForUserWrite(StringData varName);
    static void uassertValidNameForUserRead(StringData varName);

    static const Id ROOT_ID = Id(-1);

    /**
     * Use this instead of setValue for setting ROOT
     */
    void setRoot(const Document& root) {
        _root = root;
    }
    void clearRoot() {
        _root = Document();
    }
    const Document& getRoot() const {
        return _root;
    }

    void setValue(Id id, const Value& value);
    Value getValue(Id id) const;

    /**
     * returns Document() for non-document values.
     */
    Document getDocument(Id id) const;

private:
    Document _root;
    const boost::scoped_array<Value> _rest;
    const size_t _numVars;
};

/**
 * Generates Variables::Ids and keeps track of the number of Ids handed out.
 */
class VariablesIdGenerator {
public:
    VariablesIdGenerator() : _nextId(0) {}

    Variables::Id generateId() {
        return _nextId++;
    }

    /**
     * Returns the number of Ids handed out by this Generator.
     * Return value is intended to be passed to Variables constructor.
     */
    Variables::Id getIdCount() const {
        return _nextId;
    }

private:
    Variables::Id _nextId;
};

/**
 * This class represents the Variables that are defined in an Expression tree.
 *
 * All copies from a given instance share enough information to ensure unique Ids are assigned
 * and to propagate back to the original instance enough information to correctly construct a
 * Variables instance.
 */
class VariablesParseState {
public:
    explicit VariablesParseState(VariablesIdGenerator* idGenerator) : _idGenerator(idGenerator) {}

    /**
     * Assigns a named variable a unique Id. This differs from all other variables, even
     * others with the same name.
     *
     * The special variables ROOT and CURRENT are always implicitly defined with CURRENT
     * equivalent to ROOT. If CURRENT is explicitly defined by a call to this function, it
     * breaks that equivalence.
     *
     * NOTE: Name validation is responsibility of caller.
     */
    Variables::Id defineVariable(const StringData& name);

    /**
     * Returns the current Id for a variable. uasserts if the variable isn't defined.
     */
    Variables::Id getVariable(const StringData& name) const;

private:
    StringMap<Variables::Id> _variables;
    VariablesIdGenerator* _idGenerator;
};

class Expression : public IntrusiveCounterUnsigned {
public:
    virtual ~Expression(){};

    /*
      Optimize the Expression.

      This provides an opportunity to do constant folding, or to
      collapse nested operators that have the same precedence, such as
      $add, $and, or $or.

      The Expression should be replaced with the return value, which may
      or may not be the same object.  In the case of constant folding,
      a computed expression may be replaced by a constant.

      @returns the optimized Expression
     */
    virtual boost::intrusive_ptr<Expression> optimize() {
        return this;
    }

    /**
     * Add this expression's field dependencies to the set
     *
     * Expressions are trees, so this is often recursive.
     *
     * @param deps Fully qualified paths to depended-on fields are added to this set.
     *             Empty std::string means need full document.
     * @param path path to self if all ancestors are ExpressionObjects.
     *             Top-level ExpressionObject gets pointer to empty vector.
     *             If any other Expression is an ancestor, or in other cases
     *             where {a:1} inclusion objects aren't allowed, they get
     *             NULL.
     */
    virtual void addDependencies(DepsTracker* deps,
                                 std::vector<std::string>* path = NULL) const = 0;

    /** simple expressions are just inclusion exclusion as supported by ExpressionObject */
    virtual bool isSimple() {
        return false;
    }


    /**
     * Serialize the Expression tree recursively.
     * If explain is false, returns a Value parsable by parseOperand().
     */
    virtual Value serialize(bool explain) const = 0;

    /// Evaluate expression with specified inputs and return result. (only used by tests)
    Value evaluate(const Document& root) const {
        Variables vars(0, root);
        return evaluate(&vars);
    }

    /**
     * Evaluate expression with specified inputs and return result.
     *
     * While vars is non-const, if properly constructed, subexpressions modifications to it
     * should not effect outer expressions due to unique variable Ids.
     */
    Value evaluate(Variables* vars) const {
        return evaluateInternal(vars);
    }

    /*
      Utility class for parseObject() below.

      DOCUMENT_OK indicates that it is OK to use a Document in the current
      context.
     */
    class ObjectCtx {
    public:
        ObjectCtx(int options);
        static const int DOCUMENT_OK = 0x0001;
        static const int TOP_LEVEL = 0x0002;
        static const int INCLUSION_OK = 0x0004;

        bool documentOk() const;
        bool topLevel() const;
        bool inclusionOk() const;

    private:
        int options;
    };

    //
    // Diagram of relationship between parse functions when parsing a $op:
    //
    // { someFieldOrArrayIndex: { $op: [ARGS] } }
    //                             ^ parseExpression on inner $op BSONElement
    //                          ^ parseObject on BSONObject
    //             ^ parseOperand on outer BSONElement wrapping the $op Object
    //

    /**
     * Parses a BSON Object that could represent a functional expression or a Document
     * expression.
     */
    static boost::intrusive_ptr<Expression> parseObject(BSONObj obj,
                                                        ObjectCtx* pCtx,
                                                        const VariablesParseState& vps);

    /**
     * Parses a BSONElement which has already been determined to be functional expression.
     *
     * exprElement should be the only element inside the expression object. That is the
     * field name should be the $op for the expression.
     */
    static boost::intrusive_ptr<Expression> parseExpression(BSONElement exprElement,
                                                            const VariablesParseState& vps);


    /**
     * Parses a BSONElement which is an operand in an Expression.
     *
     * This is the most generic parser and can parse ExpressionFieldPath, a literal, or a $op.
     * If it is a $op, exprElement should be the outer element whose value is an Object
     * containing the $op.
     */
    static boost::intrusive_ptr<Expression> parseOperand(BSONElement exprElement,
                                                         const VariablesParseState& vps);

    /*
      Produce a field path std::string with the field prefix removed.

      Throws an error if the field prefix is not present.

      @param prefixedField the prefixed field
      @returns the field path with the prefix removed
     */
    static std::string removeFieldPrefix(const std::string& prefixedField);

    /** Evaluate the subclass Expression using the given Variables as context and return result.
     *
     *  Should only be called by subclasses, but can't be protected because they need to call
     *  this function on each other.
     */
    virtual Value evaluateInternal(Variables* vars) const = 0;

protected:
    typedef std::vector<boost::intrusive_ptr<Expression>> ExpressionVector;
};


/// Inherit from ExpressionVariadic or ExpressionFixedArity instead of directly from this class.
class ExpressionNary : public Expression {
public:
    // virtuals from Expression
    virtual boost::intrusive_ptr<Expression> optimize();
    virtual Value serialize(bool explain) const;
    virtual void addDependencies(DepsTracker* deps, std::vector<std::string>* path = NULL) const;

    /*
      Add an operand to the n-ary expression.

      @param pExpression the expression to add
    */
    virtual void addOperand(const boost::intrusive_ptr<Expression>& pExpression);

    // TODO split this into two functions
    virtual bool isAssociativeAndCommutative() const {
        return false;
    }

    /*
      Get the name of the operator.

      @returns the name of the operator; this std::string belongs to the class
        implementation, and should not be deleted
        and should not
    */
    virtual const char* getOpName() const = 0;

    /// Allow subclasses the opportunity to validate arguments at parse time.
    virtual void validateArguments(const ExpressionVector& args) const {}

    static ExpressionVector parseArguments(BSONElement bsonExpr, const VariablesParseState& vps);

protected:
    ExpressionNary() {}

    ExpressionVector vpOperand;
};

/// Inherit from ExpressionVariadic or ExpressionFixedArity instead of directly from this class.
template <typename SubClass>
class ExpressionNaryBase : public ExpressionNary {
public:
    static boost::intrusive_ptr<Expression> parse(BSONElement bsonExpr,
                                                  const VariablesParseState& vps) {
        boost::intrusive_ptr<ExpressionNaryBase> expr = new SubClass();
        ExpressionVector args = parseArguments(bsonExpr, vps);
        expr->validateArguments(args);
        expr->vpOperand = args;
        return expr;
    }
};

/// Inherit from this class if your expression takes a variable number of arguments.
template <typename SubClass>
class ExpressionVariadic : public ExpressionNaryBase<SubClass> {};

/// Inherit from this class if your expression takes a fixed number of arguments.
template <typename SubClass, int NArgs>
class ExpressionFixedArity : public ExpressionNaryBase<SubClass> {
public:
    virtual void validateArguments(const Expression::ExpressionVector& args) const {
        uassert(16020,
                mongoutils::str::stream() << "Expression " << this->getOpName() << " takes exactly "
                                          << NArgs << " arguments. " << args.size()
                                          << " were passed in.",
                args.size() == NArgs);
    }
};


class ExpressionAdd : public ExpressionVariadic<ExpressionAdd> {
public:
    // virtuals from Expression
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
    virtual bool isAssociativeAndCommutative() const {
        return true;
    }
};


class ExpressionAllElementsTrue : public ExpressionFixedArity<ExpressionAllElementsTrue, 1> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
};


class ExpressionAnd : public ExpressionVariadic<ExpressionAnd> {
public:
    // virtuals from Expression
    virtual boost::intrusive_ptr<Expression> optimize();
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
    virtual bool isAssociativeAndCommutative() const {
        return true;
    }
};


class ExpressionAnyElementTrue : public ExpressionFixedArity<ExpressionAnyElementTrue, 1> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
};


class ExpressionCoerceToBool : public Expression {
public:
    // virtuals from ExpressionNary
    virtual boost::intrusive_ptr<Expression> optimize();
    virtual void addDependencies(DepsTracker* deps, std::vector<std::string>* path = NULL) const;
    virtual Value evaluateInternal(Variables* vars) const;
    virtual Value serialize(bool explain) const;

    static boost::intrusive_ptr<ExpressionCoerceToBool> create(
        const boost::intrusive_ptr<Expression>& pExpression);


private:
    ExpressionCoerceToBool(const boost::intrusive_ptr<Expression>& pExpression);

    boost::intrusive_ptr<Expression> pExpression;
};


class ExpressionCompare : public ExpressionFixedArity<ExpressionCompare, 2> {
public:
    /** Enumeration of comparison operators. Any changes to these values require adjustment of
     *  the lookup table in the implementation.
     */
    enum CmpOp {
        EQ = 0,   // return true for a == b, false otherwise
        NE = 1,   // return true for a != b, false otherwise
        GT = 2,   // return true for a > b, false otherwise
        GTE = 3,  // return true for a >= b, false otherwise
        LT = 4,   // return true for a < b, false otherwise
        LTE = 5,  // return true for a <= b, false otherwise
        CMP = 6,  // return -1, 0, 1 for a < b, a == b, a > b
    };

    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;

    static boost::intrusive_ptr<Expression> parse(BSONElement bsonExpr,
                                                  const VariablesParseState& vps,
                                                  CmpOp cmpOp);

    ExpressionCompare(CmpOp cmpOp);

private:
    CmpOp cmpOp;
};


class ExpressionConcat : public ExpressionVariadic<ExpressionConcat> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
};


class ExpressionCond : public ExpressionFixedArity<ExpressionCond, 3> {
    typedef ExpressionFixedArity<ExpressionCond, 3> Base;

public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;

    static boost::intrusive_ptr<Expression> parse(BSONElement expr, const VariablesParseState& vps);
};


class ExpressionConstant : public Expression {
public:
    // virtuals from Expression
    virtual boost::intrusive_ptr<Expression> optimize();
    virtual void addDependencies(DepsTracker* deps, std::vector<std::string>* path = NULL) const;
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
    virtual Value serialize(bool explain) const;

    static boost::intrusive_ptr<ExpressionConstant> create(const Value& pValue);
    static boost::intrusive_ptr<Expression> parse(BSONElement bsonExpr,
                                                  const VariablesParseState& vps);

    /*
      Get the constant value represented by this Expression.

      @returns the value
     */
    Value getValue() const;

private:
    ExpressionConstant(const Value& pValue);

    Value pValue;
};

class ExpressionDateToString : public Expression {
public:
    // virtuals from Expression
    virtual boost::intrusive_ptr<Expression> optimize();
    virtual Value serialize(bool explain) const;
    virtual Value evaluateInternal(Variables* vars) const;
    virtual void addDependencies(DepsTracker* deps, std::vector<std::string>* path = NULL) const;

    static boost::intrusive_ptr<Expression> parse(BSONElement expr, const VariablesParseState& vps);

private:
    ExpressionDateToString(const std::string& format,               // the format string
                           boost::intrusive_ptr<Expression> date);  // the date to format

    // Will uassert on invalid data
    static void validateFormat(const std::string& format);

    // Need raw date as tm doesn't have millisecond resolution.
    // Format must be valid.
    static std::string formatDate(const std::string& format, const tm& tm, const long long date);

    static void insertPadded(StringBuilder& sb, int number, int spaces);

    const std::string _format;
    boost::intrusive_ptr<Expression> _date;
};

class ExpressionDayOfMonth : public ExpressionFixedArity<ExpressionDayOfMonth, 1> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;

    static inline int extract(const tm& tm) {
        return tm.tm_mday;
    }
};


class ExpressionDayOfWeek : public ExpressionFixedArity<ExpressionDayOfWeek, 1> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;

    // MySQL uses 1-7, tm uses 0-6
    static inline int extract(const tm& tm) {
        return tm.tm_wday + 1;
    }
};


class ExpressionDayOfYear : public ExpressionFixedArity<ExpressionDayOfYear, 1> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;

    // MySQL uses 1-366, tm uses 0-365
    static inline int extract(const tm& tm) {
        return tm.tm_yday + 1;
    }
};


class ExpressionDivide : public ExpressionFixedArity<ExpressionDivide, 2> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
};


class ExpressionFieldPath : public Expression {
public:
    // virtuals from Expression
    virtual boost::intrusive_ptr<Expression> optimize();
    virtual void addDependencies(DepsTracker* deps, std::vector<std::string>* path = NULL) const;
    virtual Value evaluateInternal(Variables* vars) const;
    virtual Value serialize(bool explain) const;

    /*
      Create a field path expression using old semantics (rooted off of CURRENT).

      // NOTE: this method is deprecated and only used by tests
      // TODO remove this method in favor of parse()

      Evaluation will extract the value associated with the given field
      path from the source document.

      @param fieldPath the field path string, without any leading document
        indicator
      @returns the newly created field path expression
     */
    static boost::intrusive_ptr<ExpressionFieldPath> create(const std::string& fieldPath);

    /// Like create(), but works with the raw std::string from the user with the "$" prefixes.
    static boost::intrusive_ptr<ExpressionFieldPath> parse(const std::string& raw,
                                                           const VariablesParseState& vps);

    const FieldPath& getFieldPath() const {
        return _fieldPath;
    }

private:
    ExpressionFieldPath(const std::string& fieldPath, Variables::Id variable);

    /*
      Internal implementation of evaluateInternal(), used recursively.

      The internal implementation doesn't just use a loop because of
      the possibility that we need to skip over an array.  If the path
      is "a.b.c", and a is an array, then we fan out from there, and
      traverse "b.c" for each element of a:[...].  This requires that
      a be an array of objects in order to navigate more deeply.

      @param index current path field index to extract
      @param input current document traversed to (not the top-level one)
      @returns the field found; could be an array
     */
    Value evaluatePath(size_t index, const Document& input) const;

    // Helper for evaluatePath to handle Array case
    Value evaluatePathArray(size_t index, const Value& input) const;

    const FieldPath _fieldPath;
    const Variables::Id _variable;
};


class ExpressionHour : public ExpressionFixedArity<ExpressionHour, 1> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;

    static inline int extract(const tm& tm) {
        return tm.tm_hour;
    }
};


class ExpressionIfNull : public ExpressionFixedArity<ExpressionIfNull, 2> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
};


class ExpressionLet : public Expression {
public:
    // virtuals from Expression
    virtual boost::intrusive_ptr<Expression> optimize();
    virtual Value serialize(bool explain) const;
    virtual Value evaluateInternal(Variables* vars) const;
    virtual void addDependencies(DepsTracker* deps, std::vector<std::string>* path = NULL) const;

    static boost::intrusive_ptr<Expression> parse(BSONElement expr, const VariablesParseState& vps);

    struct NameAndExpression {
        NameAndExpression() {}
        NameAndExpression(std::string name, boost::intrusive_ptr<Expression> expression)
            : name(name), expression(expression) {}

        std::string name;
        boost::intrusive_ptr<Expression> expression;
    };

    typedef std::map<Variables::Id, NameAndExpression> VariableMap;

private:
    ExpressionLet(const VariableMap& vars, boost::intrusive_ptr<Expression> subExpression);

    VariableMap _variables;
    boost::intrusive_ptr<Expression> _subExpression;
};

class ExpressionMap : public Expression {
public:
    // virtuals from Expression
    virtual boost::intrusive_ptr<Expression> optimize();
    virtual Value serialize(bool explain) const;
    virtual Value evaluateInternal(Variables* vars) const;
    virtual void addDependencies(DepsTracker* deps, std::vector<std::string>* path = NULL) const;

    static boost::intrusive_ptr<Expression> parse(BSONElement expr, const VariablesParseState& vps);

private:
    ExpressionMap(
        const std::string& varName,              // name of variable to set
        Variables::Id varId,                     // id of variable to set
        boost::intrusive_ptr<Expression> input,  // yields array to iterate
        boost::intrusive_ptr<Expression> each);  // yields results to be added to output array

    std::string _varName;
    Variables::Id _varId;
    boost::intrusive_ptr<Expression> _input;
    boost::intrusive_ptr<Expression> _each;
};

class ExpressionMeta : public Expression {
public:
    // virtuals from Expression
    virtual Value serialize(bool explain) const;
    virtual Value evaluateInternal(Variables* vars) const;
    virtual void addDependencies(DepsTracker* deps, std::vector<std::string>* path = NULL) const;

    static boost::intrusive_ptr<Expression> parse(BSONElement expr, const VariablesParseState& vps);
};

class ExpressionMillisecond : public ExpressionFixedArity<ExpressionMillisecond, 1> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;

    static int extract(const long long date);
};


class ExpressionMinute : public ExpressionFixedArity<ExpressionMinute, 1> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;

    static int extract(const tm& tm) {
        return tm.tm_min;
    }
};


class ExpressionMod : public ExpressionFixedArity<ExpressionMod, 2> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
};


class ExpressionMultiply : public ExpressionVariadic<ExpressionMultiply> {
public:
    // virtuals from Expression
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
    virtual bool isAssociativeAndCommutative() const {
        return true;
    }
};


class ExpressionMonth : public ExpressionFixedArity<ExpressionMonth, 1> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;

    // MySQL uses 1-12, tm uses 0-11
    static inline int extract(const tm& tm) {
        return tm.tm_mon + 1;
    }
};


class ExpressionNot : public ExpressionFixedArity<ExpressionNot, 1> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
};


class ExpressionObject : public Expression {
public:
    // virtuals from Expression
    virtual boost::intrusive_ptr<Expression> optimize();
    virtual bool isSimple();
    virtual void addDependencies(DepsTracker* deps, std::vector<std::string>* path = NULL) const;
    /** Only evaluates non inclusion expressions.  For inclusions, use addToDocument(). */
    virtual Value evaluateInternal(Variables* vars) const;
    virtual Value serialize(bool explain) const;

    /// like evaluate(), but return a Document instead of a Value-wrapped Document.
    Document evaluateDocument(Variables* vars) const;

    /** Evaluates with inclusions and adds results to passed in Mutable document
     *
     *  @param output the MutableDocument to add the evaluated expressions to
     *  @param currentDoc the input Document for this level (for inclusions)
     *  @param vars the variables for use in subexpressions
     */
    void addToDocument(MutableDocument& ouput, const Document& currentDoc, Variables* vars) const;

    // estimated number of fields that will be output
    size_t getSizeHint() const;

    /** Create an empty expression.
     *  Until fields are added, this will evaluate to an empty document.
     */
    static boost::intrusive_ptr<ExpressionObject> create();

    /// Like create but uses special handling of _id for root object of $project.
    static boost::intrusive_ptr<ExpressionObject> createRoot();

    /*
      Add a field to the document expression.

      @param fieldPath the path the evaluated expression will have in the
             result Document
      @param pExpression the expression to evaluate obtain this field's
             Value in the result Document
    */
    void addField(const FieldPath& fieldPath, const boost::intrusive_ptr<Expression>& pExpression);

    /*
      Add a field path to the set of those to be included.

      Note that including a nested field implies including everything on
      the path leading down to it.

      @param fieldPath the name of the field to be included
    */
    void includePath(const std::string& fieldPath);

    /*
      Get a count of the added fields.

      @returns how many fields have been added
     */
    size_t getFieldCount() const;

    /*
      Specialized BSON conversion that allows for writing out a
      $project specification.  This creates a standalone object, which must
      be added to a containing object with a name

      @param pBuilder where to write the object to
      @param requireExpression see Expression::addToBsonObj
     */
    void documentToBson(BSONObjBuilder* pBuilder, bool requireExpression) const;

    /*
      Visitor abstraction used by emitPaths().  Each path is recorded by
      calling path().
     */
    class PathSink {
    public:
        virtual ~PathSink(){};

        /**
           Record a path.

           @param path the dotted path string
           @param include if true, the path is included; if false, the path
             is excluded
         */
        virtual void path(const std::string& path, bool include) = 0;
    };

    void excludeId(bool b) {
        _excludeId = b;
    }

private:
    ExpressionObject(bool atRoot);

    // Mapping from fieldname to the Expression that generates its value.
    // NULL expression means inclusion from source document.
    typedef std::map<std::string, boost::intrusive_ptr<Expression>> FieldMap;
    FieldMap _expressions;

    // this is used to maintain order for generated fields not in the source document
    std::vector<std::string> _order;

    bool _excludeId;
    bool _atRoot;
};


class ExpressionOr : public ExpressionVariadic<ExpressionOr> {
public:
    // virtuals from Expression
    virtual boost::intrusive_ptr<Expression> optimize();
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
    virtual bool isAssociativeAndCommutative() const {
        return true;
    }
};


class ExpressionSecond : public ExpressionFixedArity<ExpressionSecond, 1> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;

    static inline int extract(const tm& tm) {
        return tm.tm_sec;
    }
};


class ExpressionSetDifference : public ExpressionFixedArity<ExpressionSetDifference, 2> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
};


class ExpressionSetEquals : public ExpressionVariadic<ExpressionSetEquals> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
    virtual void validateArguments(const ExpressionVector& args) const;
};


class ExpressionSetIntersection : public ExpressionVariadic<ExpressionSetIntersection> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
    virtual bool isAssociativeAndCommutative() const {
        return true;
    }
};


class ExpressionSetIsSubset : public ExpressionFixedArity<ExpressionSetIsSubset, 2> {
public:
    // virtuals from ExpressionNary
    virtual boost::intrusive_ptr<Expression> optimize();
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;

private:
    class Optimized;
};


class ExpressionSetUnion : public ExpressionVariadic<ExpressionSetUnion> {
public:
    // virtuals from ExpressionNary
    // virtual intrusive_ptr<Expression> optimize();
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
    virtual bool isAssociativeAndCommutative() const {
        return true;
    }
};


class ExpressionSize : public ExpressionFixedArity<ExpressionSize, 1> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
};


class ExpressionStrcasecmp : public ExpressionFixedArity<ExpressionStrcasecmp, 2> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
};


class ExpressionSubstr : public ExpressionFixedArity<ExpressionSubstr, 3> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
};


class ExpressionSubtract : public ExpressionFixedArity<ExpressionSubtract, 2> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
};


class ExpressionToLower : public ExpressionFixedArity<ExpressionToLower, 1> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
};


class ExpressionToUpper : public ExpressionFixedArity<ExpressionToUpper, 1> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;
};


class ExpressionWeek : public ExpressionFixedArity<ExpressionWeek, 1> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;

    static int extract(const tm& tm);
};


class ExpressionYear : public ExpressionFixedArity<ExpressionYear, 1> {
public:
    // virtuals from ExpressionNary
    virtual Value evaluateInternal(Variables* vars) const;
    virtual const char* getOpName() const;

    // tm_year is years since 1990
    static int extract(const tm& tm) {
        return tm.tm_year + 1900;
    }
};
}


/* ======================= INLINED IMPLEMENTATIONS ========================== */

namespace mongo {

inline Value ExpressionConstant::getValue() const {
    return pValue;
}

inline size_t ExpressionObject::getFieldCount() const {
    return _expressions.size();
}
}
