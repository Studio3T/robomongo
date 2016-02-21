/*    Copyright 2009 10gen Inc.
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

#include "robomongo/shell/db/json.h"
#include "robomongo/shell/db/ptimeutil.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

#include "mongo/db/jsobj.h"
#include "mongo/platform/cstdint.h"
#include "mongo/platform/strtoll.h"
#include "mongo/util/base64.h"
#include "mongo/util/hex.h"
#include "mongo/util/mongoutils/str.h"
#include "robomongo/core/HexUtils.h"

namespace mongo {
namespace Robomongo {

#if 0
#define MONGO_JSON_DEBUG(message) log() << "JSON DEBUG @ " << __FILE__\
    << ":" << __LINE__ << " " << __FUNCTION__ << ": " << message << endl;
#else
#define MONGO_JSON_DEBUG(message)
#endif

#define ALPHA "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
#define DIGIT "0123456789"
#define CONTROL "\a\b\f\n\r\t\v"
#define JOPTIONS "gims"

    // Size hints given to char vectors
    enum {
        ID_RESERVE_SIZE = 64,
        PAT_RESERVE_SIZE = 4096,
        OPT_RESERVE_SIZE = 64,
        FIELD_RESERVE_SIZE = 4096,
        STRINGVAL_RESERVE_SIZE = 4096,
        BINDATA_RESERVE_SIZE = 4096,
        BINDATATYPE_RESERVE_SIZE = 4096,
        NS_RESERVE_SIZE = 64
    };

    static const char* LBRACE = "{",
                 *RBRACE = "}",
                 *LBRACKET = "[",
                 *RBRACKET = "]",
                 *LPAREN = "(",
                 *RPAREN = ")",
                 *COLON = ":",
                 *COMMA = ",",
                 *FORWARDSLASH = "/",
                 *SINGLEQUOTE = "'",
                 *DOUBLEQUOTE = "\"";

    JParse::JParse(const char* str)
        : _buf(str), _input(str), _input_end(str + strlen(str)) {}

    Status JParse::parseError(const StringData& msg) {
        std::ostringstream ossmsg;
        ossmsg << msg;
#ifndef ROBOMONGO // offset will be taken using public offset() method.
        ossmsg << ": offset:";
        ossmsg << offset();
#endif
        return Status(ErrorCodes::FailedToParse, ossmsg.str());
    }

    Status JParse::value(const StringData& fieldName, BSONObjBuilder& builder) {
        MONGO_JSON_DEBUG("fieldName: " << fieldName);
        if (accept(LBRACE, false)) {
            Status ret = object(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        else if (accept(LBRACKET, false)) {
            Status ret = array(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        else if (accept("new")) {
            Status ret = constructor(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        else if (accept("Date")) {
            Status ret = date(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
#ifdef ROBOMONGO
        else if (accept("ISODate")) {
            Status ret = isodate(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        else if (accept("NumberLong")) {
            Status ret = numberLong(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        else if (accept("UUID")) {
            Status ret = uuid(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        else if (accept("LUUID")) {
            Status ret = luuid(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        else if (accept("JUUID")) {
            Status ret = juuid(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        else if (accept("NUUID") || accept("CSUUID")) {
            Status ret = nuuid(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        else if (accept("PYUUID")) {
            Status ret = pyuuid(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
#endif
        else if (accept("Timestamp")) {
            Status ret = timestamp(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        else if (accept("ObjectId")) {
            Status ret = objectId(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        else if (accept("Dbref") || accept("DBRef")) {
            Status ret = dbRef(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        else if (accept(FORWARDSLASH, false)) {
            Status ret = regex(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        else if (accept(DOUBLEQUOTE, false) || accept(SINGLEQUOTE, false)) {
            std::string valueString;
            valueString.reserve(STRINGVAL_RESERVE_SIZE);
            Status ret = quotedString(&valueString);
            if (ret != Status::OK()) {
                return ret;
            }
            builder.append(fieldName, valueString);
        }
        else if (accept("true")) {
            builder.append(fieldName, true);
        }
        else if (accept("false")) {
            builder.append(fieldName, false);
        }
        else if (accept("null")) {
            builder.appendNull(fieldName);
        }
        else if (accept("undefined")) {
            builder.appendUndefined(fieldName);
        }
        else if (accept("NaN")) {
            builder.append(fieldName, std::numeric_limits<double>::quiet_NaN());
        }
        else if (accept("Infinity")) {
            builder.append(fieldName, std::numeric_limits<double>::infinity());
        }
        else if (accept("-Infinity")) {
            builder.append(fieldName, -std::numeric_limits<double>::infinity());
        }
        else {
            Status ret = number(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        return Status::OK();
    }

    Status JParse::object(const StringData& fieldName, BSONObjBuilder& builder, bool subObject) {
        MONGO_JSON_DEBUG("fieldName: " << fieldName);
        if (!accept(LBRACE)) {
            return parseError("Expecting '{'");
        }

        // Empty object
        if (accept(RBRACE)) {
            if (subObject) {
                BSONObjBuilder empty(builder.subobjStart(fieldName));
                empty.done();
            }
            return Status::OK();
        }

        // Special object
        std::string firstField;
        firstField.reserve(FIELD_RESERVE_SIZE);
        Status ret = field(&firstField);
        if (ret != Status::OK()) {
            return ret;
        }

        if (firstField == "$oid") {
            if (!subObject) {
                return parseError("Reserved field name in base object: $oid");
            }
            Status ret = objectIdObject(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        else if (firstField == "$binary") {
            if (!subObject) {
                return parseError("Reserved field name in base object: $binary");
            }
            Status ret = binaryObject(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        else if (firstField == "$date") {
            if (!subObject) {
                return parseError("Reserved field name in base object: $date");
            }
            Status ret = dateObject(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        else if (firstField == "$timestamp") {
            if (!subObject) {
                return parseError("Reserved field name in base object: $timestamp");
            }
            Status ret = timestampObject(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        else if (firstField == "$regex") {
            if (!subObject) {
                return parseError("Reserved field name in base object: $regex");
            }
            Status ret = regexObject(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        else if (firstField == "$ref") {
            if (!subObject) {
                return parseError("Reserved field name in base object: $ref");
            }
            Status ret = dbRefObject(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        else if (firstField == "$undefined") {
            if (!subObject) {
                return parseError("Reserved field name in base object: $undefined");
            }
            Status ret = undefinedObject(fieldName, builder);
            if (ret != Status::OK()) {
                return ret;
            }
        }
        else { // firstField != <reserved field name>
            // Normal object

            // Only create a sub builder if this is not the base object
            BSONObjBuilder* objBuilder = &builder;
            scoped_ptr<BSONObjBuilder> subObjBuilder;
            if (subObject) {
                subObjBuilder.reset(new BSONObjBuilder(builder.subobjStart(fieldName)));
                objBuilder = subObjBuilder.get();
            }

            if (!accept(COLON)) {
                return parseError("Expecting ':'");
            }
            Status valueRet = value(firstField, *objBuilder);
            if (valueRet != Status::OK()) {
                return valueRet;
            }
            while (accept(COMMA)) {
                std::string fieldName;
                fieldName.reserve(FIELD_RESERVE_SIZE);
                Status fieldRet = field(&fieldName);
                if (fieldRet != Status::OK()) {
                    return fieldRet;
                }
                if (!accept(COLON)) {
                    return parseError("Expecting ':'");
                }
                Status valueRet = value(fieldName, *objBuilder);
                if (valueRet != Status::OK()) {
                    return valueRet;
                }
            }
        }
        if (!accept(RBRACE)) {
            return parseError("Expecting '}' or ','");
        }
        return Status::OK();
    }

    Status JParse::objectIdObject(const StringData& fieldName, BSONObjBuilder& builder) {
        if (!accept(COLON)) {
            return parseError("Expected ':'");
        }
        std::string id;
        id.reserve(ID_RESERVE_SIZE);
        Status ret = quotedString(&id);
        if (ret != Status::OK()) {
            return ret;
        }
        if (id.size() != 24) {
            return parseError("Expecting 24 hex digits: " + id);
        }
        if (!isHexString(id)) {
            return parseError("Expecting hex digits: " + id);
        }
        builder.append(fieldName, OID(id));
        return Status::OK();
    }

    Status JParse::binaryObject(const StringData& fieldName, BSONObjBuilder& builder) {
        if (!accept(COLON)) {
            return parseError("Expected ':'");
        }
        std::string binDataString;
        binDataString.reserve(BINDATA_RESERVE_SIZE);
        Status dataRet = quotedString(&binDataString);
        if (dataRet != Status::OK()) {
            return dataRet;
        }
        if (binDataString.size() % 4 != 0) {
            return parseError("Invalid length base64 encoded string");
        }
        if (!isBase64String(binDataString)) {
            return parseError("Invalid character in base64 encoded string");
        }
        const std::string& binData = base64::decode(binDataString);
        if (!accept(COMMA)) {
            return parseError("Expected ','");
        }

        if (!acceptField("$type")) {
            return parseError("Expected second field name: \"$type\", in \"$binary\" object");
        }
        if (!accept(COLON)) {
            return parseError("Expected ':'");
        }
        std::string binDataType;
        binDataType.reserve(BINDATATYPE_RESERVE_SIZE);
        Status typeRet = quotedString(&binDataType);
        if (typeRet != Status::OK()) {
            return typeRet;
        }
        if ((binDataType.size() != 2) || !isHexString(binDataType)) {
            return parseError("Argument of $type in $bindata object must be a hex string representation of a single byte");
        }
        builder.appendBinData( fieldName, binData.length(),
                BinDataType(fromHex(binDataType)),
                binData.data());
        return Status::OK();
    }

    Status JParse::dateObject(const StringData& fieldName, BSONObjBuilder& builder) {
        if (!accept(COLON)) {
            return parseError("Expected ':'");
        }
        errno = 0;
        char* endptr;
        int64_t millis = strtoll(_input, &endptr, 10);
        if (errno == ERANGE) {
            return parseError("Date milliseconds overflow");
        }
        if (_input == endptr) {
            return parseError("Date expecting integer milliseconds");
        }
        _input = endptr;
        builder.appendDate(fieldName, millis);
        return Status::OK();
    }

    Status JParse::timestampObject(const StringData& fieldName, BSONObjBuilder& builder) {
        if (!accept(COLON)) {
            return parseError("Expecting ':'");
        }
        if (!accept(LBRACE)) {
            return parseError("Expecting '{' to start \"$timestamp\" object");
        }

        if (!acceptField("t")) {
            return parseError("Expected field name \"t\" in \"$timestamp\" sub object");
        }
        if (!accept(COLON)) {
            return parseError("Expecting ':'");
        }
        if (accept("-")) {
            return parseError("Negative seconds in \"$timestamp\"");
        }
        errno = 0;
        char* endptr;
        uint32_t seconds = strtoul(_input, &endptr, 10);
        if (errno == ERANGE) {
            return parseError("Timestamp seconds overflow");
        }
        if (_input == endptr) {
            return parseError("Expecting unsigned integer seconds in \"$timestamp\"");
        }
        _input = endptr;
        if (!accept(COMMA)) {
            return parseError("Expecting ','");
        }

        if (!acceptField("i")) {
            return parseError("Expected field name \"i\" in \"$timestamp\" sub object");
        }
        if (!accept(COLON)) {
            return parseError("Expecting ':'");
        }
        if (accept("-")) {
            return parseError("Negative increment in \"$timestamp\"");
        }
        errno = 0;
        uint32_t count = strtoul(_input, &endptr, 10);
        if (errno == ERANGE) {
            return parseError("Timestamp increment overflow");
        }
        if (_input == endptr) {
            return parseError("Expecting unsigned integer increment in \"$timestamp\"");
        }
        _input = endptr;

        if (!accept(RBRACE)) {
            return parseError("Expecting '}'");
        }
        builder.appendTimestamp(fieldName, (static_cast<uint64_t>(seconds))*1000, count);
        return Status::OK();
    }

    Status JParse::regexObject(const StringData& fieldName, BSONObjBuilder& builder) {
        if (!accept(COLON)) {
            return parseError("Expecting ':'");
        }
        std::string pat;
        pat.reserve(PAT_RESERVE_SIZE);
        Status patRet = quotedString(&pat);
        if (patRet != Status::OK()) {
            return patRet;
        }
        if (accept(COMMA)) {
            if (!acceptField("$options")) {
                return parseError("Expected field name: \"$options\" in \"$regex\" object");
            }
            if (!accept(COLON)) {
                return parseError("Expecting ':'");
            }
            std::string opt;
            opt.reserve(OPT_RESERVE_SIZE);
            Status optRet = quotedString(&opt);
            if (optRet != Status::OK()) {
                return optRet;
            }
            Status optCheckRet = regexOptCheck(opt);
            if (optCheckRet != Status::OK()) {
                return optCheckRet;
            }
            builder.appendRegex(fieldName, pat, opt);
        }
        else {
            builder.appendRegex(fieldName, pat, "");
        }
        return Status::OK();
    }

    Status JParse::dbRefObject(const StringData& fieldName, BSONObjBuilder& builder) {
        BSONObjBuilder subBuilder(builder.subobjStart(fieldName));

        if (!accept(COLON)) {
            return parseError("DBRef: Expecting ':'");
        }
        std::string ns;
        ns.reserve(NS_RESERVE_SIZE);
        Status ret = quotedString(&ns);
        if (ret != Status::OK()) {
            return ret;
        }
        subBuilder.append("$ref", ns);

        if (!accept(COMMA)) {
            return parseError("DBRef: Expecting ','");
        }

        if (!acceptField("$id")) {
            return parseError("DBRef: Expected field name: \"$id\" in \"$ref\" object");
        }
        if (!accept(COLON)) {
            return parseError("DBRef: Expecting ':'");
        }
        Status valueRet = value("$id", subBuilder);
        if (valueRet != Status::OK()) {
            return valueRet;
        }

#ifdef ROBOMONGO
        // $db field
        // Optional.
        // Contains the name of the database where the referenced document resides.
        // Only some drivers support $db references.

        if (accept(COMMA)) {
            if (!acceptField("$db")) {
                return parseError("DBRef: Expected optional field name: \"$db\" in \"DBRef\" object. Remove comma after $id field to make $db field optional");
            }

            if (!accept(COLON)) {
                return parseError("DBRef: Expecting ':' after $db field");
            }

            Status valueRet = value("$db", subBuilder);
            if (valueRet != Status::OK()) {
                return valueRet;
            }
        }
#endif

        subBuilder.done();
        return Status::OK();
    }

    Status JParse::undefinedObject(const StringData& fieldName, BSONObjBuilder& builder) {
        if (!accept(COLON)) {
            return parseError("Expecting ':'");
        }
        if (!accept("true")) {
            return parseError("Reserved field \"$undefined\" requires value of true");
        }
        builder.appendUndefined(fieldName);
        return Status::OK();
    }

    Status JParse::array(const StringData& fieldName, BSONObjBuilder& builder) {
        MONGO_JSON_DEBUG("fieldName: " << fieldName);
        uint32_t index(0);
        if (!accept(LBRACKET)) {
            return parseError("Expecting '['");
        }
        BSONObjBuilder subBuilder(builder.subarrayStart(fieldName));
        if (!accept(RBRACKET, false)) {
            do {
                Status ret = value(builder.numStr(index), subBuilder);
                if (ret != Status::OK()) {
                    return ret;
                }
                index++;
            } while (accept(COMMA));
        }
        subBuilder.done();
        if (!accept(RBRACKET)) {
            return parseError("Expecting ']' or ','");
        }
        return Status::OK();
    }

    /* NOTE: this could be easily modified to allow "new" before other
     * constructors, but for now it only allows "new" before Date().
     * Also note that unlike the interactive shell "Date(x)" and "new Date(x)"
     * have the same behavior.  XXX: this may not be desired. */
    Status JParse::constructor(const StringData& fieldName, BSONObjBuilder& builder) {
        if (accept("Date")) {
            date(fieldName, builder);
        }
        else {
            return parseError("\"new\" keyword not followed by Date constructor");
        }
        return Status::OK();
    }

    Status JParse::date(const StringData& fieldName, BSONObjBuilder& builder) {
        if (!accept(LPAREN)) {
            return parseError("Expecting '('");
        }
        errno = 0;
        char* endptr;
        int64_t millis = strtoll(_input, &endptr, 10);
        if (errno == ERANGE) {
            return parseError("Date milliseconds overflow");
        }
        if (_input == endptr) {
            return parseError("Date expecting integer milliseconds");
        }
        _input = endptr;
        if (!accept(RPAREN)) {
            return parseError("Expecting ')'");
        }
        builder.appendDate(fieldName, millis);
        return Status::OK();
    }

#ifdef ROBOMONGO
    Status JParse::isodate(const StringData& fieldName, BSONObjBuilder& builder) {
        if (!accept(LPAREN)) {
            return parseError("Expecting '('");
        }

        std::string datestr;
        Status ret = quotedString(&datestr);
        if (ret != Status::OK()) {
            return ret;
        }

        bool isSuccessfull = false;
        boost::posix_time::ptime isotime = miutil::ptimeFromIsoString(datestr,isSuccessfull);
        if (!isSuccessfull) {
            return parseError("Invalid date format");
        }

        boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
        boost::posix_time::time_duration diff = isotime - epoch;
        int64_t millis = diff.total_milliseconds();

        if (!accept(RPAREN)) {
            return parseError("Expecting ')'");
        }
        builder.appendDate(fieldName, millis);
        return Status::OK();
    }

    Status JParse::numberLong(const StringData &fieldName, BSONObjBuilder &builder)
    {
        if (!accept(LPAREN)) {
            return parseError("Expecting '('");
        }

        //
        // Below we have almost exact copy of the JParse::number method.
        //

        char* endptrll;
        char* endptrd;
        long long retll;
        double retd;

        // reset errno to make sure that we are getting it from strtod
        errno = 0;
        retd = strtod(_input, &endptrd);
        // if pointer does not move, we found no digits
        if (_input == endptrd) {
            return parseError("Bad characters in value");
        }
        if (errno == ERANGE) {
            return parseError("Value cannot fit in double");
        }
        // reset errno to make sure that we are getting it from strtoll
        errno = 0;
        retll = strtoll(_input, &endptrll, 10);
        if (endptrll < endptrd || errno == ERANGE) {
            // The number either had characters only meaningful for a double or
            // could not fit in a 64 bit int
            MONGO_JSON_DEBUG("Type: double");
            return parseError("The number either had characters only meaningful for a double or could not fit in a 64 bit int");
        }
        else {
            // The number can fit in a 64 bit int
            MONGO_JSON_DEBUG("Type: 64 bit int");
            builder.append(fieldName, retll);
        }
        _input = endptrd;
        if (_input >= _input_end) {
            return parseError("Trailing number at end of input");
        }

        if (!accept(RPAREN)) {
            return parseError("Expecting ')'");
        }

        return Status::OK();
    }

    Status JParse::uuid(const StringData &fieldName, BSONObjBuilder &builder)
    {
        return parseUuid(fieldName, builder, newUUID, ::Robomongo::DefaultEncoding);
    }

    Status JParse::luuid(const StringData &fieldName, BSONObjBuilder &builder)
    {
        return parseUuid(fieldName, builder, bdtUUID, ::Robomongo::DefaultEncoding);
    }

    Status JParse::juuid(const StringData &fieldName, BSONObjBuilder &builder)
    {
        return parseUuid(fieldName, builder, bdtUUID, ::Robomongo::JavaLegacy);
    }

    Status JParse::nuuid(const StringData &fieldName, BSONObjBuilder &builder)
    {
        return parseUuid(fieldName, builder, bdtUUID, ::Robomongo::CSharpLegacy);
    }

    Status JParse::pyuuid(const StringData &fieldName, BSONObjBuilder &builder)
    {
        return parseUuid(fieldName, builder, bdtUUID, ::Robomongo::PythonLegacy);
    }

    Status JParse::parseUuid(const StringData &fieldName, BSONObjBuilder &builder, BinDataType binType, ::Robomongo::UUIDEncoding uuidEncoding)
    {
        if (!accept(LPAREN)) {
            return parseError("Expecting '('");
        }

        std::string datastr;
        Status ret = quotedString(&datastr);
        if (ret != Status::OK()) {
            return ret;
        }

        std::string hex = ::Robomongo::HexUtils::uuidToHex(datastr, uuidEncoding);

        if (hex.empty() || !::Robomongo::HexUtils::isHexString(hex)) {
            return parseError("Invalid hex string for UUID");
        }

        int len;
        boost::scoped_array<const char> data(::Robomongo::HexUtils::fromHex(hex, &len));
        if (data == NULL)
            return parseError("Invalid UUID");

        if (!accept(RPAREN))
            return parseError("Expecting ')'");

        builder.appendBinData(fieldName, len,
                binType,
                data.get());

        return Status::OK();
    }
#endif

    Status JParse::timestamp(const StringData& fieldName, BSONObjBuilder& builder) {
        if (!accept(LPAREN)) {
            return parseError("Expecting '('");
        }
        if (accept("-")) {
            return parseError("Negative seconds in \"$timestamp\"");
        }
        errno = 0;
        char* endptr;
        uint32_t seconds = strtoul(_input, &endptr, 10);
        if (errno == ERANGE) {
            return parseError("Timestamp seconds overflow");
        }
        if (_input == endptr) {
            return parseError("Expecting unsigned integer seconds in \"$timestamp\"");
        }
        _input = endptr;
        if (!accept(COMMA)) {
            return parseError("Expecting ','");
        }
        if (accept("-")) {
            return parseError("Negative seconds in \"$timestamp\"");
        }
        errno = 0;
        uint32_t count = strtoul(_input, &endptr, 10);
        if (errno == ERANGE) {
            return parseError("Timestamp increment overflow");
        }
        if (_input == endptr) {
            return parseError("Expecting unsigned integer increment in \"$timestamp\"");
        }
        _input = endptr;
        if (!accept(RPAREN)) {
            return parseError("Expecting ')'");
        }
        builder.appendTimestamp(fieldName, (static_cast<uint64_t>(seconds))*1000, count);
        return Status::OK();
    }

    Status JParse::objectId(const StringData& fieldName, BSONObjBuilder& builder) {
        if (!accept(LPAREN)) {
            return parseError("Expecting '('");
        }
        std::string id;
        id.reserve(ID_RESERVE_SIZE);
        Status ret = quotedString(&id);
        if (ret != Status::OK()) {
            return ret;
        }
        if (!accept(RPAREN)) {
            return parseError("Expecting ')'");
        }
        if (id.size() != 24) {
            return parseError("Expecting 24 hex digits: " + id);
        }
        if (!isHexString(id)) {
            return parseError("Expecting hex digits: " + id);
        }
        builder.append(fieldName, OID(id));
        return Status::OK();
    }

    Status JParse::dbRef(const StringData& fieldName, BSONObjBuilder& builder) {
        BSONObjBuilder subBuilder(builder.subobjStart(fieldName));

        if (!accept(LPAREN)) {
            return parseError("Expecting '('");
        }
        std::string ns;
        ns.reserve(NS_RESERVE_SIZE);
        Status refRet = quotedString(&ns);
        if (refRet != Status::OK()) {
            return refRet;
        }
        subBuilder.append("$ref", ns);

        if (!accept(COMMA)) {
            return parseError("Expecting ','");
        }

        Status valueRet = value("$id", subBuilder);
        if (valueRet != Status::OK()) {
            return valueRet;
        }

        if (!accept(RPAREN)) {
            return parseError("Expecting ')'");
        }

        subBuilder.done();
        return Status::OK();
    }

    Status JParse::regex(const StringData& fieldName, BSONObjBuilder& builder) {
        if (!accept(FORWARDSLASH)) {
            return parseError("Expecting '/'");
        }
        std::string pat;
        pat.reserve(PAT_RESERVE_SIZE);
        Status patRet = regexPat(&pat);
        if (patRet != Status::OK()) {
            return patRet;
        }
        if (!accept(FORWARDSLASH)) {
            return parseError("Expecting '/'");
        }
        std::string opt;
        opt.reserve(OPT_RESERVE_SIZE);
        Status optRet = regexOpt(&opt);
        if (optRet != Status::OK()) {
            return optRet;
        }
        Status optCheckRet = regexOptCheck(opt);
        if (optCheckRet != Status::OK()) {
            return optCheckRet;
        }
        builder.appendRegex(fieldName, pat, opt);
        return Status::OK();
    }

    Status JParse::regexPat(std::string* result) {
        MONGO_JSON_DEBUG("");
        return chars(result, "/");
    }

    Status JParse::regexOpt(std::string* result) {
        MONGO_JSON_DEBUG("");
        return chars(result, "", JOPTIONS);
    }

    Status JParse::regexOptCheck(const StringData& opt) {
        MONGO_JSON_DEBUG("opt: " << opt);
        std::size_t i;
        for (i = 0; i < opt.size(); i++) {
            if (!match(opt[i], JOPTIONS)) {
                return parseError(string("Bad regex option: ") + opt[i]);
            }
        }
        return Status::OK();
    }

    Status JParse::number(const StringData& fieldName, BSONObjBuilder& builder) {
        char* endptrll;
        char* endptrd;
        long long retll;
        double retd;

        // reset errno to make sure that we are getting it from strtod
        errno = 0;
        retd = strtod(_input, &endptrd);
        // if pointer does not move, we found no digits
        if (_input == endptrd) {
            return parseError("Bad characters in value");
        }
        if (errno == ERANGE) {
            return parseError("Value cannot fit in double");
        }
        // reset errno to make sure that we are getting it from strtoll
        errno = 0;
        retll = strtoll(_input, &endptrll, 10);
        if (endptrll < endptrd || errno == ERANGE) {
            // The number either had characters only meaningful for a double or
            // could not fit in a 64 bit int
            MONGO_JSON_DEBUG("Type: double");
            builder.append(fieldName, retd);
        }
        else if (retll == static_cast<int>(retll)) {
            // The number can fit in a 32 bit int
            MONGO_JSON_DEBUG("Type: 32 bit int");
            builder.append(fieldName, static_cast<int>(retll));
        }
        else {
            // The number can fit in a 64 bit int
            MONGO_JSON_DEBUG("Type: 64 bit int");
            builder.append(fieldName, retll);
        }
        _input = endptrd;
        if (_input >= _input_end) {
            return parseError("Trailing number at end of input");
        }
        return Status::OK();
    }

    Status JParse::field(std::string* result) {
        MONGO_JSON_DEBUG("");
        if (accept(DOUBLEQUOTE, false) || accept(SINGLEQUOTE, false)) {
            // Quoted key
            // TODO: make sure quoted field names cannot contain null characters
            return quotedString(result);
        }
        else {
            // Unquoted key
            while (_input < _input_end && isspace(*_input)) ++_input;
            if (_input >= _input_end) {
                return parseError("Field name expected");
            }
            if (!match(*_input, ALPHA "_$")) {
                return parseError("First character in field must be [A-Za-z$_]");
            }
            return chars(result, "", ALPHA DIGIT "_$");
        }
    }

    Status JParse::quotedString(std::string* result) {
        MONGO_JSON_DEBUG("");
        if (accept(DOUBLEQUOTE, true)) {
            Status ret = chars(result, "\"");
            if (ret != Status::OK()) {
                return ret;
            }
            if (!accept(DOUBLEQUOTE)) {
                return parseError("Expecting '\"'");
            }
        }
        else if (accept(SINGLEQUOTE, true)) {
            Status ret = chars(result, "'");
            if (ret != Status::OK()) {
                return ret;
            }
            if (!accept(SINGLEQUOTE)) {
                return parseError("Expecting '''");
            }
        }
        else {
            return parseError("Expecting quoted string");
        }
        return Status::OK();
    }

    /*
     * terminalSet are characters that signal end of string (e.g.) [ :\0]
     * allowedSet are the characters that are allowed, if this is set
     */
    Status JParse::chars(std::string* result, const char* terminalSet,
            const char* allowedSet) {
        MONGO_JSON_DEBUG("terminalSet: " << terminalSet);
        if (_input >= _input_end) {
            return parseError("Unexpected end of input");
        }
        const char* q = _input;
        while (q < _input_end && !match(*q, terminalSet)) {
            MONGO_JSON_DEBUG("q: " << q);
            if (allowedSet != NULL) {
                if (!match(*q, allowedSet)) {
                    _input = q;
                    return Status::OK();
                }
            }
            if (0x00 <= *q && *q <= 0x1F) {
                return parseError("Invalid control character");
            }
            if (*q == '\\' && q + 1 < _input_end) {
                switch (*(++q)) {
                    // Escape characters allowed by the JSON spec
                    case '"':  result->push_back('"');  break;
                    case '\'': result->push_back('\''); break;
                    case '\\': result->push_back('\\'); break;
                    case '/':  result->push_back('/');  break;
                    case 'b':  result->push_back('\b'); break;
                    case 'f':  result->push_back('\f'); break;
                    case 'n':  result->push_back('\n'); break;
                    case 'r':  result->push_back('\r'); break;
                    case 't':  result->push_back('\t'); break;
                    case 'u': { //expect 4 hexdigits
                                  // TODO: handle UTF-16 surrogate characters
                                  ++q;
                                  if (q + 4 >= _input_end) {
                                      return parseError("Expecting 4 hex digits");
                                  }
                                  if (!isHexString(StringData(q, 4))) {
                                      return parseError("Expecting 4 hex digits");
                                  }
                                  unsigned char first = fromHex(q);
                                  unsigned char second = fromHex(q += 2);
                                  const std::string& utf8str = encodeUTF8(first, second);
                                  for (unsigned int i = 0; i < utf8str.size(); i++) {
                                      result->push_back(utf8str[i]);
                                  }
                                  ++q;
                                  break;
                              }
                               // Vertical tab character.  Not in JSON spec but allowed in
                               // our implementation according to test suite.
                    case 'v':  result->push_back('\v'); break;
                               // Escape characters we explicity disallow
                    case 'x':  return parseError("Hex escape not supported");
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':  return parseError("Octal escape not supported");
                               // By default pass on the unescaped character
                    default:   result->push_back(*q); break;
                    // TODO: check for escaped control characters
                }
                ++q;
            }
            else {
                result->push_back(*q++);
            }
        }
        if (q < _input_end) {
            _input = q;
            return Status::OK();
        }
        return parseError("Unexpected end of input");
    }

    std::string JParse::encodeUTF8(unsigned char first, unsigned char second) const {
        std::ostringstream oss;
        if (first == 0 && second < 0x80) {
            oss << second;
        }
        else if (first < 0x08) {
            oss << char( 0xc0 | (first << 2 | second >> 6) );
            oss << char( 0x80 | (~0xc0 & second) );
        }
        else {
            oss << char( 0xe0 | (first >> 4) );
            oss << char( 0x80 | (~0xc0 & (first << 2 | second >> 6) ) );
            oss << char( 0x80 | (~0xc0 & second) );
        }
        return oss.str();
    }

    bool JParse::accept(const char* token, bool advance) {
        MONGO_JSON_DEBUG("token: " << token);
        const char* check = _input;
        if (token == NULL) {
            return false;
        }
        while (check < _input_end && isspace(*check)) {
            ++check;
        }
        while (*token != '\0') {
            if (check >= _input_end) {
                return false;
            }
            if (*token++ != *check++) {
                return false;
            }
        }
        if (advance) { _input = check; }
        return true;
    }

    bool JParse::acceptField(const StringData& expectedField) {
        MONGO_JSON_DEBUG("expectedField: " << expectedField);
        std::string nextField;
        nextField.reserve(FIELD_RESERVE_SIZE);
        Status ret = field(&nextField);
        if (ret != Status::OK()) {
            return false;
        }
        if (expectedField != nextField) {
            return false;
        }
        return true;
    }

    inline bool JParse::match(char matchChar, const char* matchSet) const {
        if (matchSet == NULL) {
            return true;
        }
        if (*matchSet == '\0') {
            return false;
        }
        return (strchr(matchSet, matchChar) != NULL);
    }

    bool JParse::isHexString(const StringData& str) const {
        MONGO_JSON_DEBUG("str: " << str);
        std::size_t i;
        for (i = 0; i < str.size(); i++) {
            if (!isxdigit(str[i])) {
                return false;
            }
        }
        return true;
    }

    bool JParse::isBase64String(const StringData& str) const {
        MONGO_JSON_DEBUG("str: " << str);
        std::size_t i;
        for (i = 0; i < str.size(); i++) {
            if (!match(str[i], base64::chars)) {
                return false;
            }
        }
        return true;
    }

    BSONObj fromjson(const char* jsonString, int* len) {
        MONGO_JSON_DEBUG("jsonString: " << jsonString);
        if (jsonString[0] == '\0') {
            if (len) *len = 0;
            return BSONObj();
        }
        JParse jparse(jsonString);
        BSONObjBuilder builder;
        Status ret = jparse.object("UNUSED", builder, false);
        if (ret != Status::OK()) {
            ostringstream message;
            message << "code " << ret.code() << ": " << ret.codeString() << ": " << ret.reason();

            /*std::string strReason = ret.reason();
            char *charReason = new char[strReason.size() + 1];
            std::copy(strReason.begin(), strReason.end(), charReason);
            charReason[strReason.size()] = '\0';
            */

            throw ParseMsgAssertionException(16619, message.str(), jparse.offset(), ret.reason());
        }
        if (len) *len = jparse.offset();
        return builder.obj();
    }

    BSONObj fromjson(const std::string& str) {
        return fromjson( str.c_str() );
    }

}  /* namespace Robomongo */
}  /* namespace mongo */
