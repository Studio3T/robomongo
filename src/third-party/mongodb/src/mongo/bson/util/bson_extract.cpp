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

#include "mongo/bson/util/bson_extract.h"

#include "mongo/db/jsobj.h"
#include "mongo/util/mongoutils/str.h"

namespace mongo {

Status bsonExtractField(const BSONObj& object,
                        const StringData& fieldName,
                        BSONElement* outElement) {
    BSONElement element = object.getField(fieldName);
    if (element.eoo())
        return Status(ErrorCodes::NoSuchKey,
                      mongoutils::str::stream() << "Missing expected field \""
                                                << fieldName.toString() << "\"");
    *outElement = element;
    return Status::OK();
}

Status bsonExtractTypedField(const BSONObj& object,
                             const StringData& fieldName,
                             BSONType type,
                             BSONElement* outElement) {
    Status status = bsonExtractField(object, fieldName, outElement);
    if (!status.isOK())
        return status;
    if (type != outElement->type()) {
        return Status(ErrorCodes::TypeMismatch,
                      mongoutils::str::stream()
                          << "\"" << fieldName << "\" had the wrong type. Expected "
                          << typeName(type) << ", found " << typeName(outElement->type()));
    }
    return Status::OK();
}

Status bsonExtractBooleanField(const BSONObj& object, const StringData& fieldName, bool* out) {
    BSONElement element;
    Status status = bsonExtractTypedField(object, fieldName, Bool, &element);
    if (!status.isOK())
        return status;
    *out = element.boolean();
    return Status::OK();
}

Status bsonExtractBooleanFieldWithDefault(const BSONObj& object,
                                          const StringData& fieldName,
                                          bool defaultValue,
                                          bool* out) {
    BSONElement value;
    Status status = bsonExtractField(object, fieldName, &value);
    if (status == ErrorCodes::NoSuchKey) {
        *out = defaultValue;
        return Status::OK();
    } else if (!status.isOK()) {
        return status;
    } else if (!value.isNumber() && !value.isBoolean()) {
        return Status(ErrorCodes::TypeMismatch,
                      mongoutils::str::stream() << "Expected boolean or number type for field \""
                                                << fieldName << "\", found "
                                                << typeName(value.type()));
    } else {
        *out = value.trueValue();
        return Status::OK();
    }
}

Status bsonExtractStringField(const BSONObj& object,
                              const StringData& fieldName,
                              std::string* out) {
    BSONElement element;
    Status status = bsonExtractTypedField(object, fieldName, String, &element);
    if (!status.isOK())
        return status;
    *out = element.str();
    return Status::OK();
}

Status bsonExtractOpTimeField(const BSONObj& object, const StringData& fieldName, OpTime* out) {
    BSONElement element;
    Status status = bsonExtractTypedField(object, fieldName, Timestamp, &element);
    if (!status.isOK())
        return status;
    *out = element._opTime();
    return Status::OK();
}

Status bsonExtractOIDField(const BSONObj& object, const StringData& fieldName, OID* out) {
    BSONElement element;
    Status status = bsonExtractTypedField(object, fieldName, jstOID, &element);
    if (!status.isOK())
        return status;
    *out = element.OID();
    return Status::OK();
}

Status bsonExtractOIDFieldWithDefault(const BSONObj& object,
                                      const StringData& fieldName,
                                      const OID& defaultValue,
                                      OID* out) {
    Status status = bsonExtractOIDField(object, fieldName, out);
    if (status == ErrorCodes::NoSuchKey) {
        *out = defaultValue;
    } else if (!status.isOK()) {
        return status;
    }
    return Status::OK();
}

Status bsonExtractStringFieldWithDefault(const BSONObj& object,
                                         const StringData& fieldName,
                                         const StringData& defaultValue,
                                         std::string* out) {
    Status status = bsonExtractStringField(object, fieldName, out);
    if (status == ErrorCodes::NoSuchKey) {
        *out = defaultValue.toString();
    } else if (!status.isOK()) {
        return status;
    }
    return Status::OK();
}

Status bsonExtractIntegerField(const BSONObj& object, const StringData& fieldName, long long* out) {
    BSONElement value;
    Status status = bsonExtractField(object, fieldName, &value);
    if (!status.isOK())
        return status;
    if (!value.isNumber()) {
        return Status(ErrorCodes::TypeMismatch,
                      mongoutils::str::stream() << "Expected field \"" << fieldName
                                                << "\" to have numeric type, but found "
                                                << typeName(value.type()));
    }
    long long result = value.safeNumberLong();
    if (result != value.numberDouble()) {
        return Status(ErrorCodes::BadValue,
                      mongoutils::str::stream()
                          << "Expected field \"" << fieldName
                          << "\" to have a value "
                             "exactly representable as a 64-bit integer, but found " << value);
    }
    *out = result;
    return Status::OK();
}

Status bsonExtractIntegerFieldWithDefault(const BSONObj& object,
                                          const StringData& fieldName,
                                          long long defaultValue,
                                          long long* out) {
    Status status = bsonExtractIntegerField(object, fieldName, out);
    if (status == ErrorCodes::NoSuchKey) {
        *out = defaultValue;
        status = Status::OK();
    }
    return status;
}

}  // namespace mongo
