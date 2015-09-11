// @file bsonobj.h

/*    Copyright 2009 10gen Inc.
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

#pragma once

#include <set>
#include <list>
#include <string>
#include <vector>
#include <utility>

#include "mongo/bson/bsonelement.h"
#include "mongo/base/data_view.h"
#include "mongo/base/string_data.h"
#include "mongo/bson/util/builder.h"
#include "mongo/client/export_macros.h"
#include "mongo/platform/atomic_word.h"
#include "mongo/util/bufreader.h"
#include "mongo/util/shared_buffer.h"

namespace mongo {

typedef std::set<BSONElement, BSONElementCmpWithoutField> BSONElementSet;
typedef std::multiset<BSONElement, BSONElementCmpWithoutField> BSONElementMSet;

/**
   C++ representation of a "BSON" object -- that is, an extended JSON-style
   object in a binary representation.

   See bsonspec.org.

   Note that BSONObj's have a smart pointer capability built in -- so you can
   pass them around by value.  The reference counts used to implement this
   do not use locking, so copying and destroying BSONObj's are not thread-safe
   operations.

 BSON object format:

 code
 <unsigned totalSize> {<byte BSONType><cstring FieldName><Data>}* EOO

 totalSize includes itself.

 Data:
 Bool:      <byte>
 EOO:       nothing follows
 Undefined: nothing follows
 OID:       an OID object
 NumberDouble: <double>
 NumberInt: <int32>
 String:    <unsigned32 strsizewithnull><cstring>
 Date:      <8bytes>
 Regex:     <cstring regex><cstring options>
 Object:    a nested object, leading with its entire size, which terminates with EOO.
 Array:     same as object
 DBRef:     <strlen> <cstring ns> <oid>
 DBRef:     a database reference: basically a collection name plus an Object ID
 BinData:   <int len> <byte subtype> <byte[len] data>
 Code:      a function (not a closure): same format as String.
 Symbol:    a language symbol (say a python symbol).  same format as String.
 Code With Scope: <total size><String><Object>
 \endcode
 */
class MONGO_CLIENT_API BSONObj {
public:
    /** Construct an empty BSONObj -- that is, {}. */
    BSONObj() {
        // Little endian ordering here, but that is ok regardless as BSON is spec'd to be
        // little endian external to the system. (i.e. the rest of the implementation of
        // bson, not this part, fails to support big endian)
        static const char kEmptyObjectPrototype[] = {/*size*/ 5, 0, 0, 0, /*eoo*/ 0};
        _objdata = kEmptyObjectPrototype;
    }

    /** Construct a BSONObj from data in the proper format.
     *  Use this constructor when something else owns bsonData's buffer
    */
    explicit BSONObj(const char* bsonData) {
        init(bsonData);
    }

    explicit BSONObj(SharedBuffer ownedBuffer)
        : _objdata(ownedBuffer.get() ? ownedBuffer.get() : BSONObj().objdata()),
          _ownedBuffer(ownedBuffer.moveFrom()) {}

#if __cplusplus >= 201103L
    /** Move construct a BSONObj */
    BSONObj(BSONObj&& other)
        : _objdata(std::move(other._objdata)), _ownedBuffer(std::move(other._ownedBuffer)) {
        other._objdata = BSONObj()._objdata;  // To return to an empty state.
        dassert(!other.isOwned());
    }

    // The explicit move constructor above will inhibit generation of the copy ctor, so
    // explicitly request the default implementation.

    /** Copy construct a BSONObj. */
    BSONObj(const BSONObj&) = default;
#endif

    /** Provide assignment semantics. We use the value taking form so that we can use copy
     *  and swap, and consume both lvalue and rvalue references.
     */
    BSONObj& operator=(BSONObj otherCopy) {
        this->swap(otherCopy);
        return *this;
    }

    /** Swap this BSONObj with 'other' */
    void swap(BSONObj& other) {
        using std::swap;
        swap(_objdata, other._objdata);
        swap(_ownedBuffer, other._ownedBuffer);
    }

    /**
       A BSONObj can use a buffer it "owns" or one it does not.

       OWNED CASE
       If the BSONObj owns the buffer, the buffer can be shared among several BSONObj's (by
       assignment). In this case the buffer is basically implemented as a shared_ptr.
       Since BSONObj's are typically immutable, this works well.

       UNOWNED CASE
       A BSONObj can also point to BSON data in some other data structure it does not "own" or free
       later. For example, in a memory mapped file.  In this case, it is important the original data
       stays in scope for as long as the BSONObj is in use.  If you think the original data may go
       out of scope, call BSONObj::getOwned() to promote your BSONObj to having its own copy.

       On a BSONObj assignment, if the source is unowned, both the source and dest will have unowned
       pointers to the original buffer after the assignment.

       If you are not sure about ownership but need the buffer to last as long as the BSONObj, call
       getOwned().  getOwned() is a no-op if the buffer is already owned.  If not already owned, a
       malloc and memcpy will result.

       Most ways to create BSONObj's create 'owned' variants.  Unowned versions can be created with:
       (1) specifying true for the ifree parameter in the constructor
       (2) calling BSONObjBuilder::done().  Use BSONObjBuilder::obj() to get an owned copy
       (3) retrieving a subobject retrieves an unowned pointer into the parent BSON object

       @return true if this is in owned mode
    */
    bool isOwned() const {
        return _ownedBuffer.get() != 0;
    }

    /** assure the data buffer is under the control of this BSONObj and not a remote buffer
        @see isOwned()
    */
    BSONObj getOwned() const;

    /** @return a new full (and owned) copy of the object. */
    BSONObj copy() const;

    /** Readable representation of a BSON object in an extended JSON-style notation.
        This is an abbreviated representation which might be used for logging.
    */
    enum { maxToStringRecursionDepth = 100 };

    std::string toString(bool isArray = false, bool full = false) const;
    void toString(StringBuilder& s, bool isArray = false, bool full = false, int depth = 0) const;

    /** Properly formatted JSON string.
        @param pretty if true we try to add some lf's and indentation
    */
    std::string jsonString(JsonStringFormat format = Strict,
                           int pretty = 0,
                           bool isArray = false) const;

    /** note: addFields always adds _id even if not specified */
    int addFields(BSONObj& from, std::set<std::string>& fields); /* returns n added */

    /** remove specified field and return a new object with the remaining fields.
        slowish as builds a full new object
     */
    BSONObj removeField(const StringData& name) const;

    /** returns # of top level fields in the object
       note: iterates to count the fields
    */
    int nFields() const;

    /** adds the field names to the fields set.  does NOT clear it (appends). */
    int getFieldNames(std::set<std::string>& fields) const;

    /** @return the specified element.  element.eoo() will be true if not found.
        @param name field to find. supports dot (".") notation to reach into embedded objects.
         for example "x.y" means "in the nested object in field x, retrieve field y"
    */
    BSONElement getFieldDotted(const StringData& name) const;

    /** Like getFieldDotted(), but expands arrays and returns all matching objects.
     *  Turning off expandLastArray allows you to retrieve nested array objects instead of
     *  their contents.
     */
    void getFieldsDotted(const StringData& name,
                         BSONElementSet& ret,
                         bool expandLastArray = true) const;
    void getFieldsDotted(const StringData& name,
                         BSONElementMSet& ret,
                         bool expandLastArray = true) const;

    /** Like getFieldDotted(), but returns first array encountered while traversing the
        dotted fields of name.  The name variable is updated to represent field
        names with respect to the returned element. */
    BSONElement getFieldDottedOrArray(const char*& name) const;

    /** Get the field of the specified name. eoo() is true on the returned
        element if not found.
    */
    BSONElement getField(const StringData& name) const;

    /** Get several fields at once. This is faster than separate getField() calls as the size of
        elements iterated can then be calculated only once each.
        @param n number of fieldNames, and number of elements in the fields array
        @param fields if a field is found its element is stored in its corresponding position in
                this array. if not found the array element is unchanged.
     */
    void getFields(unsigned n, const char** fieldNames, BSONElement* fields) const;

    /** Get the field of the specified name. eoo() is true on the returned
        element if not found.
    */
    BSONElement operator[](const StringData& field) const {
        return getField(field);
    }

    BSONElement operator[](int field) const {
        StringBuilder ss;
        ss << field;
        std::string s = ss.str();
        return getField(s.c_str());
    }

    /** @return true if field exists */
    bool hasField(const StringData& name) const {
        return !getField(name).eoo();
    }
    /** @return true if field exists */
    bool hasElement(const StringData& name) const {
        return hasField(name);
    }

    /** @return "" if DNE or wrong type */
    const char* getStringField(const StringData& name) const;

    /** @return subobject of the given name */
    BSONObj getObjectField(const StringData& name) const;

    /** @return INT_MIN if not present - does some type conversions */
    int getIntField(const StringData& name) const;

    /** @return false if not present
        @see BSONElement::trueValue()
     */
    bool getBoolField(const StringData& name) const;

    /** @param pattern a BSON obj indicating a set of (un-dotted) field
     *  names.  Element values are ignored.
     *  @return a BSON obj constructed by taking the elements of this obj
     *  that correspond to the fields in pattern. Field names of the
     *  returned object are replaced with the empty string. If field in
     *  pattern is missing, it is omitted from the returned object.
     *
     *  Example: if this = {a : 4 , b : 5 , c : 6})
     *    this.extractFieldsUnDotted({a : 1 , c : 1}) -> {"" : 4 , "" : 6 }
     *    this.extractFieldsUnDotted({b : "blah"}) -> {"" : 5}
     *
    */
    BSONObj extractFieldsUnDotted(const BSONObj& pattern) const;

    /** extract items from object which match a pattern object.
        e.g., if pattern is { x : 1, y : 1 }, builds an object with
        x and y elements of this object, if they are present.
       returns elements with original field names
    */
    BSONObj extractFields(const BSONObj& pattern, bool fillWithNull = false) const;

    BSONObj filterFieldsUndotted(const BSONObj& filter, bool inFilter) const;

    BSONElement getFieldUsingIndexNames(const StringData& fieldName, const BSONObj& indexKey) const;

    /** arrays are bson objects with numeric and increasing field names
        @return true if field names are numeric and increasing
     */
    bool couldBeArray() const;

    /** @return the raw data of the object */
    const char* objdata() const {
        return _objdata;
    }

    /** @return total size of the BSON object in bytes */
    int objsize() const {
        return ConstDataView(objdata()).readLE<int>();
    }

    /** performs a cursory check on the object's size only. */
    bool isValid() const {
        int x = objsize();
        return x > 0 && x <= BSONObjMaxInternalSize;
    }

    /** @return ok if it can be stored as a valid embedded doc.
     *  Not valid if any field name:
     *      - contains a "."
     *      - starts with "$"
     *          -- unless it is a dbref ($ref/$id/[$db]/...)
     */
    inline bool okForStorage() const {
        return _okForStorage(false, true).isOK();
    }

    /** Same as above with the following extra restrictions
     *  Not valid if:
     *      - "_id" field is a
     *          -- Regex
     *          -- Array
     */
    inline bool okForStorageAsRoot() const {
        return _okForStorage(true, true).isOK();
    }

    /**
     * Validates that this can be stored as an embedded document
     * See details above in okForStorage
     *
     * If 'deep' is true then validation is done to children
     *
     * If not valid a user readable status message is returned.
     */
    inline Status storageValidEmbedded(const bool deep = true) const {
        return _okForStorage(false, deep);
    }

    /**
     * Validates that this can be stored as a document (in a collection)
     * See details above in okForStorageAsRoot
     *
     * If 'deep' is true then validation is done to children
     *
     * If not valid a user readable status message is returned.
     */
    inline Status storageValid(const bool deep = true) const {
        return _okForStorage(true, deep);
    }

    /** @return true if object is empty -- i.e.,  {} */
    bool isEmpty() const {
        return objsize() <= 5;
    }

    void dump() const;

    /** Alternative output format */
    std::string hexDump() const;

    /**wo='well ordered'.  fields must be in same order in each object.
       Ordering is with respect to the signs of the elements
       and allows ascending / descending key mixing.
       @return  <0 if l<r. 0 if l==r. >0 if l>r
    */
    int woCompare(const BSONObj& r, const Ordering& o, bool considerFieldName = true) const;

    /**wo='well ordered'.  fields must be in same order in each object.
       Ordering is with respect to the signs of the elements
       and allows ascending / descending key mixing.
       @return  <0 if l<r. 0 if l==r. >0 if l>r
    */
    int woCompare(const BSONObj& r,
                  const BSONObj& ordering = BSONObj(),
                  bool considerFieldName = true) const;

    bool operator<(const BSONObj& other) const {
        return woCompare(other) < 0;
    }
    bool operator<=(const BSONObj& other) const {
        return woCompare(other) <= 0;
    }
    bool operator>(const BSONObj& other) const {
        return woCompare(other) > 0;
    }
    bool operator>=(const BSONObj& other) const {
        return woCompare(other) >= 0;
    }

    /**
     * @param useDotted whether to treat sort key fields as possibly dotted and expand into them
     */
    int woSortOrder(const BSONObj& r, const BSONObj& sortKey, bool useDotted = false) const;

    bool equal(const BSONObj& r) const;

    /**
     * Functor compatible with std::hash for std::unordered_{map,set}
     * Warning: The hash function is subject to change. Do not use in cases where hashes need
     *          to be consistent across versions.
     */
    struct Hasher {
        size_t operator()(const BSONObj& obj) const;
    };

    /**
     * @param otherObj
     * @return true if 'this' is a prefix of otherObj- in other words if
     * otherObj contains the same field names and field vals in the same
     * order as 'this', plus optionally some additional elements.
     */
    bool isPrefixOf(const BSONObj& otherObj) const;

    /**
     * @param otherObj
     * @return returns true if the list of field names in 'this' is a prefix
     * of the list of field names in otherObj.  Similar to 'isPrefixOf',
     * but ignores the field values and only looks at field names.
     */
    bool isFieldNamePrefixOf(const BSONObj& otherObj) const;

    /** This is "shallow equality" -- ints and doubles won't match.  for a
       deep equality test use woCompare (which is slower).
    */
    bool binaryEqual(const BSONObj& r) const {
        int os = objsize();
        if (os == r.objsize()) {
            return (os == 0 || memcmp(objdata(), r.objdata(), os) == 0);
        }
        return false;
    }

    /** @return first field of the object */
    BSONElement firstElement() const {
        return BSONElement(objdata() + 4);
    }

    /** faster than firstElement().fieldName() - for the first element we can easily find the
     * fieldname without computing the element size.
    */
    const char* firstElementFieldName() const {
        const char* p = objdata() + 4;
        return *p == EOO ? "" : p + 1;
    }

    BSONType firstElementType() const {
        const char* p = objdata() + 4;
        return (BSONType)*p;
    }

    /** Get the _id field from the object.  For good performance drivers should
        assure that _id is the first element of the object; however, correct operation
        is assured regardless.
        @return true if found
    */
    bool getObjectID(BSONElement& e) const;

    // Return a version of this object where top level elements of types
    // that are not part of the bson wire protocol are replaced with
    // std::string identifier equivalents.
    // TODO Support conversion of element types other than min and max.
    BSONObj clientReadable() const;

    /** Return new object with the field names replaced by those in the
        passed object. */
    BSONObj replaceFieldNames(const BSONObj& obj) const;

    /** true unless corrupt */
    bool valid() const;

    bool operator==(const BSONObj& other) const {
        return equal(other);
    }
    bool operator!=(const BSONObj& other) const {
        return !operator==(other);
    }

    enum MatchType {
        Equality = 0,
        LT = 0x1,
        LTE = 0x3,
        GTE = 0x6,
        GT = 0x4,
        opIN = 0x8,  // { x : { $in : [1,2,3] } }
        NE = 0x9,
        opSIZE = 0x0A,
        opALL = 0x0B,
        NIN = 0x0C,
        opEXISTS = 0x0D,
        opMOD = 0x0E,
        opTYPE = 0x0F,
        opREGEX = 0x10,
        opOPTIONS = 0x11,
        opELEM_MATCH = 0x12,
        opNEAR = 0x13,
        opWITHIN = 0x14,
        opMAX_DISTANCE = 0x15,
        opGEO_INTERSECTS = 0x16,
    };

    /** add all elements of the object to the specified vector */
    void elems(std::vector<BSONElement>&) const;
    /** add all elements of the object to the specified list */
    void elems(std::list<BSONElement>&) const;

    friend class BSONObjIterator;
    typedef BSONObjIterator iterator;

    /** use something like this:
        for( BSONObj::iterator i = myObj.begin(); i.more(); ) {
            BSONElement e = i.next();
            ...
        }
    */
    BSONObjIterator begin() const;

    void appendSelfToBufBuilder(BufBuilder& b) const {
        verify(objsize());
        b.appendBuf(objdata(), objsize());
    }

    template <typename T>
    bool coerceVector(std::vector<T>* out) const;

    typedef SharedBuffer::Holder Holder;

    /** Given a pointer to a region of un-owned memory containing BSON data, prefixed by
     *  sufficient space for a BSONObj::Holder object, return a BSONObj that owns the
     *  memory.
     *
     * This class will call free(holderPrefixedData), so it must have been allocated in a way
     * that makes that valid.
     */
    static BSONObj takeOwnership(char* holderPrefixedData) {
        return BSONObj(SharedBuffer::takeOwnership(holderPrefixedData));
    }

    /// members for Sorter
    struct SorterDeserializeSettings {};  // unused
    void serializeForSorter(BufBuilder& buf) const {
        buf.appendBuf(objdata(), objsize());
    }
    static BSONObj deserializeForSorter(BufReader& buf, const SorterDeserializeSettings&) {
        const int size = buf.peek<int>();
        const void* ptr = buf.skip(size);
        return BSONObj(static_cast<const char*>(ptr));
    }
    int memUsageForSorter() const {
        // TODO consider ownedness?
        return sizeof(BSONObj) + objsize();
    }

private:
    void _assertInvalid() const;

    void init(const char* data) {
        _objdata = data;
        if (!isValid())
            _assertInvalid();
    }

    /**
     * Validate if the element is okay to be stored in a collection, maybe as the root element
     *
     * If 'root' is true then checks against _id are made.
     * If 'deep' is false then do not traverse through children
     */
    Status _okForStorage(bool root, bool deep) const;

    const char* _objdata;
    SharedBuffer _ownedBuffer;
};

std::ostream& operator<<(std::ostream& s, const BSONObj& o);
std::ostream& operator<<(std::ostream& s, const BSONElement& e);

StringBuilder& operator<<(StringBuilder& s, const BSONObj& o);
StringBuilder& operator<<(StringBuilder& s, const BSONElement& e);

inline void swap(BSONObj& l, BSONObj& r) {
    l.swap(r);
}

struct BSONArray : BSONObj {
    // Don't add anything other than forwarding constructors!!!
    BSONArray() : BSONObj() {}
    explicit BSONArray(const BSONObj& obj) : BSONObj(obj) {}
};
}
