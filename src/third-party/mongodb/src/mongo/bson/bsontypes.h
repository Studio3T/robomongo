// bsontypes.h

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

#include "mongo/util/assert_util.h"

namespace mongo {

class BSONArrayBuilder;
class BSONElement;
class BSONObj;
class BSONObjBuilder;
class BSONObjBuilderValueStream;
class BSONObjIterator;
class Ordering;
struct BSONArray;  // empty subclass of BSONObj useful for overloading
struct BSONElementCmpWithoutField;

extern BSONObj maxKey;
extern BSONObj minKey;

/**
    the complete list of valid BSON types
    see also bsonspec.org
*/
enum BSONType {
    /** smaller than all other types */
    MinKey = -1,
    /** end of object */
    EOO = 0,
    /** double precision floating point value */
    NumberDouble = 1,
    /** character string, stored in utf8 */
    String = 2,
    /** an embedded object */
    Object = 3,
    /** an embedded array */
    Array = 4,
    /** binary data */
    BinData = 5,
    /** Undefined type */
    Undefined = 6,
    /** ObjectId */
    jstOID = 7,
    /** boolean type */
    Bool = 8,
    /** date type */
    Date = 9,
    /** null type */
    jstNULL = 10,
    /** regular expression, a pattern with options */
    RegEx = 11,
    /** deprecated / will be redesigned */
    DBRef = 12,
    /** deprecated / use CodeWScope */
    Code = 13,
    /** a programming language (e.g., Python) symbol */
    Symbol = 14,
    /** javascript code that can execute on the database server, with SavedContext */
    CodeWScope = 15,
    /** 32 bit signed integer */
    NumberInt = 16,
    /** Updated to a Date with value next OpTime on insert */
    Timestamp = 17,
    /** 64 bit integer */
    NumberLong = 18,
    /** max type that is not MaxKey */
    JSTypeMax = 18,
    /** larger than all other types */
    MaxKey = 127
};

/**
 * returns the name of the argument's type
 */
const char* typeName(BSONType type);

/* subtypes of BinData.
   bdtCustom and above are ones that the JS compiler understands, but are
   opaque to the database.
*/
enum BinDataType {
    BinDataGeneral = 0,
    Function = 1,
    ByteArrayDeprecated = 2, /* use BinGeneral instead */
    bdtUUID = 3,             /* deprecated */
    newUUID = 4,             /* language-independent UUID format across all drivers */
    MD5Type = 5,
    bdtCustom = 128
};

/** Returns a number for where a given type falls in the sort order.
 *  Elements with the same return value should be compared for value equality.
 *  The return value is not a BSONType and should not be treated as one.
 *  Note: if the order changes, indexes have to be re-built or than can be corruption
 */
inline int canonicalizeBSONType(BSONType type) {
    switch (type) {
        case MinKey:
        case MaxKey:
            return type;
        case EOO:
        case Undefined:
            return 0;
        case jstNULL:
            return 5;
        case NumberDouble:
        case NumberInt:
        case NumberLong:
            return 10;
        case mongo::String:
        case Symbol:
            return 15;
        case Object:
            return 20;
        case mongo::Array:
            return 25;
        case BinData:
            return 30;
        case jstOID:
            return 35;
        case mongo::Bool:
            return 40;
        case mongo::Date:
            return 45;
        case Timestamp:
            return 47;
        case RegEx:
            return 50;
        case DBRef:
            return 55;
        case Code:
            return 60;
        case CodeWScope:
            return 65;
        default:
            verify(0);
            return -1;
    }
}
}
