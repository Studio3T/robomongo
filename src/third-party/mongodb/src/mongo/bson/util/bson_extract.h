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

#pragma once

#include <string>
#include <vector>

#include "mongo/base/status.h"
#include "mongo/base/string_data.h"
#include "mongo/bson/bsontypes.h"

namespace mongo {

class BSONObj;
class BSONElement;
class OID;
class OpTime;

/**
 * Finds an element named "fieldName" in "object".
 *
 * Returns Status::OK() and sets "*outElement" to the found element on success.  Returns
 * ErrorCodes::NoSuchKey if there are no matches.
 */
Status bsonExtractField(const BSONObj& object,
                        const StringData& fieldName,
                        BSONElement* outElement);

/**
 * Finds an element named "fieldName" in "object".
 *
 * Returns Status::OK() and sets *outElement to the found element on success.  Returns
 * ErrorCodes::NoSuchKey if there are no matches for "fieldName", and ErrorCodes::TypeMismatch
 * if the type of the matching element is not "type".  For return values other than
 * Status::OK(), the resulting value of "*outElement" is undefined.
 */
Status bsonExtractTypedField(const BSONObj& object,
                             const StringData& fieldName,
                             BSONType type,
                             BSONElement* outElement);

/**
 * Finds a bool-like element named "fieldName" in "object".
 *
 * Returns Status::OK() and sets *out to the found element's boolean value on success.  Returns
 * ErrorCodes::NoSuchKey if there are no matches for "fieldName", and ErrorCodes::TypeMismatch
 * if the type of the matching element is not Bool or a number type.  For return values other
 * than Status::OK(), the resulting value of "*out" is undefined.
 */
Status bsonExtractBooleanField(const BSONObj& object, const StringData& fieldName, bool* out);

/**
 * Finds an element named "fieldName" in "object" that represents an integral value.
 *
 * Returns Status::OK() and sets *out to the element's 64-bit integer value representation on
 * success.  Returns ErrorCodes::NoSuchKey if there are no matches for "fieldName".  Returns
 * ErrorCodes::TypeMismatch if the value of the matching element is not of a numeric type.
 * Returns ErrorCodes::BadValue if the value does not have an exact 64-bit integer
 * representation.  For return values other than Status::OK(), the resulting value of "*out" is
 * undefined.
 */
Status bsonExtractIntegerField(const BSONObj& object, const StringData& fieldName, long long* out);

/**
 * Finds a string-typed element named "fieldName" in "object" and stores its value in "out".
 *
 * Returns Status::OK() and sets *out to the found element's std::string value on success.  Returns
 * ErrorCodes::NoSuchKey if there are no matches for "fieldName", and ErrorCodes::TypeMismatch
 * if the type of the matching element is not String.  For return values other than
 * Status::OK(), the resulting value of "*out" is undefined.
 */
Status bsonExtractStringField(const BSONObj& object, const StringData& fieldName, std::string* out);

/**
 * Finds an OpTime-typed element named "fieldName" in "object" and stores its value in "out".
 *
 * Returns Status::OK() and sets *out to the found element's OpTime value on success.  Returns
 * ErrorCodes::NoSuchKey if there are no matches for "fieldName", and ErrorCodes::TypeMismatch
 * if the type of the matching element is not OpTime.  For return values other than
 * Status::OK(), the resulting value of "*out" is undefined.
 */
Status bsonExtractOpTimeField(const BSONObj& object, const StringData& fieldName, OpTime* out);

/**
 * Finds an OID-typed element named "fieldName" in "object" and stores its value in "out".
 *
 * Returns Status::OK() and sets *out to the found element's OID value on success.  Returns
 * ErrorCodes::NoSuchKey if there are no matches for "fieldName", and ErrorCodes::TypeMismatch
 * if the type of the matching element is not OID.  For return values other than Status::OK(),
 * the resulting value of "*out" is undefined.
 */
Status bsonExtractOIDField(const BSONObj& object, const StringData& fieldName, OID* out);

/**
 * Finds a bool-like element named "fieldName" in "object".
 *
 * If a field named "fieldName" is present, and is either a number or boolean type, stores the
 * truth value of the field into "*out".  If no field named "fieldName" is present, sets "*out"
 * to "defaultValue".  In these cases, returns Status::OK().
 *
 * If "fieldName" is present more than once, behavior is undefined.  If the found field is not a
 * boolean or number, returns ErrorCodes::TypeMismatch.
 */
Status bsonExtractBooleanFieldWithDefault(const BSONObj& object,
                                          const StringData& fieldName,
                                          bool defaultValue,
                                          bool* out);

/**
 * Finds an element named "fieldName" in "object" that represents an integral value.
 *
 * If a field named "fieldName" is present and is a value of numeric type with an exact 64-bit
 * integer representation, returns that representation in *out and returns Status::OK().  If
 * there is no field named "fieldName", stores defaultValue into *out and returns Status::OK().
 * If the field is found, but has non-numeric type, returns ErrorCodes::TypeMismatch.  If the
 * value has numeric type, but cannot be represented as a 64-bit integer, returns
 * ErrorCodes::BadValue.
 */
Status bsonExtractIntegerFieldWithDefault(const BSONObj& object,
                                          const StringData& fieldName,
                                          long long defaultValue,
                                          long long* out);

/**
 * Finds a std::string element named "fieldName" in "object".
 *
 * If a field named "fieldName" is present, and is a string, stores the value of the field into
 * "*out".  If no field named fieldName is present, sets "*out" to "defaultValue".  In these
 * cases, returns Status::OK().
 *
 * If "fieldName" is present more than once, behavior is undefined.  If the found field is not a
 * string, returns ErrorCodes::TypeMismatch.
 */
Status bsonExtractStringFieldWithDefault(const BSONObj& object,
                                         const StringData& fieldName,
                                         const StringData& defaultValue,
                                         std::string* out);

/**
 * Finds an OID-typed element named "fieldName" in "object" and stores its value in *out.
 *
 * Returns Status::OK() and sets *out to the found element's OID value on success.  If no field
 * named "fieldName" is present, *out is set to "defaultValue" and Status::OK() is returned.
 * Returns ErrorCodes::TypeMismatch if the type of the matching element is not OID.  For return
 * values other than Status::OK(), the resulting value of *out is undefined.
 */
Status bsonExtractOIDFieldWithDefault(const BSONObj& object,
                                      const StringData& fieldName,
                                      const OID& defaultValue,
                                      OID* out);

}  // namespace mongo
