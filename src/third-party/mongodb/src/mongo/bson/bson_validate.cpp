// bson_validate.cpp

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

#include <cstring>
#include <deque>
#include <limits>

#include "mongo/base/data_view.h"
#include "mongo/bson/bson_validate.h"
#include "mongo/bson/oid.h"
#include "mongo/db/jsobj.h"

namespace mongo {

namespace {

/**
 * Creates a status with InvalidBSON code and adds information about _id if available.
 * WARNING: only pass in a non-EOO idElem if it has been fully validated already!
 */
Status makeError(std::string baseMsg, BSONElement idElem) {
    if (idElem.eoo()) {
        baseMsg += " in object with unknown _id";
    } else {
        baseMsg += " in object with " + idElem.toString(/*field name=*/true, /*full=*/true);
    }
    return Status(ErrorCodes::InvalidBSON, baseMsg);
}

class Buffer {
public:
    Buffer(const char* buffer, uint64_t maxLength)
        : _buffer(buffer), _position(0), _maxLength(maxLength) {}

    template <typename N>
    bool readNumber(N* out) {
        if ((_position + sizeof(N)) > _maxLength)
            return false;
        if (out) {
            *out = ConstDataView(_buffer).readLE<N>(_position);
        }
        _position += sizeof(N);
        return true;
    }

    Status readCString(StringData* out) {
        const void* x = memchr(_buffer + _position, 0, _maxLength - _position);
        if (!x)
            return makeError("no end of c-string", _idElem);
        uint64_t len = static_cast<uint64_t>(static_cast<const char*>(x) - (_buffer + _position));

        StringData data(_buffer + _position, len);
        _position += len + 1;

        if (out) {
            *out = data;
        }
        return Status::OK();
    }

    Status readUTF8String(StringData* out) {
        int sz;
        if (!readNumber<int>(&sz))
            return makeError("invalid bson", _idElem);

        if (sz <= 0) {
            // must have NULL at the very least
            return makeError("invalid bson", _idElem);
        }

        if (out) {
            *out = StringData(_buffer + _position, sz);
        }

        if (!skip(sz - 1))
            return makeError("invalid bson", _idElem);

        char c;
        if (!readNumber<char>(&c))
            return makeError("invalid bson", _idElem);

        if (c != 0)
            return makeError("not null terminated string", _idElem);

        return Status::OK();
    }

    bool skip(uint64_t sz) {
        _position += sz;
        return _position < _maxLength;
    }

    uint64_t position() const {
        return _position;
    }

    const char* getBasePtr() const {
        return _buffer;
    }

    /**
     * WARNING: only pass in a non-EOO idElem if it has been fully validated already!
     */
    void setIdElem(BSONElement idElem) {
        _idElem = idElem;
    }

private:
    const char* _buffer;
    uint64_t _position;
    uint64_t _maxLength;
    BSONElement _idElem;
};

struct ValidationState {
    enum State { BeginObj = 1, WithinObj, EndObj, BeginCodeWScope, EndCodeWScope, Done };
};

class ValidationObjectFrame {
public:
    int startPosition() const {
        return _startPosition & ~(1 << 31);
    }
    bool isCodeWithScope() const {
        return _startPosition & (1 << 31);
    }

    void setStartPosition(int pos) {
        _startPosition = (_startPosition & (1 << 31)) | (pos & ~(1 << 31));
    }
    void setIsCodeWithScope(bool isCodeWithScope) {
        if (isCodeWithScope) {
            _startPosition |= 1 << 31;
        } else {
            _startPosition &= ~(1 << 31);
        }
    }

    int expectedSize;

private:
    int _startPosition;
};

/**
 * WARNING: only pass in a non-EOO idElem if it has been fully validated already!
 */
Status validateElementInfo(Buffer* buffer, ValidationState::State* nextState, BSONElement idElem) {
    Status status = Status::OK();

    signed char type;
    if (!buffer->readNumber<signed char>(&type))
        return makeError("invalid bson", idElem);

    if (type == EOO) {
        *nextState = ValidationState::EndObj;
        return Status::OK();
    }

    StringData name;
    status = buffer->readCString(&name);
    if (!status.isOK())
        return status;

    switch (type) {
        case MinKey:
        case MaxKey:
        case jstNULL:
        case Undefined:
            return Status::OK();

        case jstOID:
            if (!buffer->skip(OID::kOIDSize))
                return makeError("invalid bson", idElem);
            return Status::OK();

        case NumberInt:
            if (!buffer->skip(sizeof(int32_t)))
                return makeError("invalid bson", idElem);
            return Status::OK();

        case Bool:
            if (!buffer->skip(sizeof(int8_t)))
                return makeError("invalid bson", idElem);
            return Status::OK();


        case NumberDouble:
        case NumberLong:
        case Timestamp:
        case Date:
            if (!buffer->skip(sizeof(int64_t)))
                return makeError("invalid bson", idElem);
            return Status::OK();

        case DBRef:
            status = buffer->readUTF8String(NULL);
            if (!status.isOK())
                return status;
            buffer->skip(OID::kOIDSize);
            return Status::OK();

        case RegEx:
            status = buffer->readCString(NULL);
            if (!status.isOK())
                return status;
            status = buffer->readCString(NULL);
            if (!status.isOK())
                return status;

            return Status::OK();

        case Code:
        case Symbol:
        case String:
            status = buffer->readUTF8String(NULL);
            if (!status.isOK())
                return status;
            return Status::OK();

        case BinData: {
            int sz;
            if (!buffer->readNumber<int>(&sz))
                return makeError("invalid bson", idElem);
            if (sz < 0 || sz == std::numeric_limits<int>::max())
                return makeError("invalid size in bson", idElem);
            if (!buffer->skip(1 + sz))
                return makeError("invalid bson", idElem);
            return Status::OK();
        }
        case CodeWScope:
            *nextState = ValidationState::BeginCodeWScope;
            return Status::OK();
        case Object:
        case Array:
            *nextState = ValidationState::BeginObj;
            return Status::OK();

        default:
            return makeError("invalid bson type", idElem);
    }
}

Status validateBSONIterative(Buffer* buffer) {
    std::deque<ValidationObjectFrame> frames;
    ValidationObjectFrame* curr = NULL;
    ValidationState::State state = ValidationState::BeginObj;

    uint64_t idElemStartPos = 0;  // will become idElem once validated
    BSONElement idElem;

    while (state != ValidationState::Done) {
        switch (state) {
            case ValidationState::BeginObj:
                frames.push_back(ValidationObjectFrame());
                curr = &frames.back();
                curr->setStartPosition(buffer->position());
                curr->setIsCodeWithScope(false);
                if (!buffer->readNumber<int>(&curr->expectedSize)) {
                    return makeError("bson size is larger than buffer size", idElem);
                }
                state = ValidationState::WithinObj;
            // fall through
            case ValidationState::WithinObj: {
                const bool atTopLevel = frames.size() == 1;
                // check if we've finished validating idElem and are at start of next element.
                if (atTopLevel && idElemStartPos) {
                    idElem = BSONElement(buffer->getBasePtr() + idElemStartPos);
                    buffer->setIdElem(idElem);
                    idElemStartPos = 0;
                }

                const uint64_t elemStartPos = buffer->position();
                ValidationState::State nextState = state;
                Status status = validateElementInfo(buffer, &nextState, idElem);
                if (!status.isOK())
                    return status;

                // we've already validated that fieldname is safe to access as long as we aren't
                // at the end of the object, since EOO doesn't have a fieldname.
                if (nextState != ValidationState::EndObj && idElem.eoo() && atTopLevel) {
                    if (strcmp(buffer->getBasePtr() + elemStartPos + 1 /*type*/, "_id") == 0) {
                        idElemStartPos = elemStartPos;
                    }
                }

                state = nextState;
                break;
            }
            case ValidationState::EndObj: {
                int actualLength = buffer->position() - curr->startPosition();
                if (actualLength != curr->expectedSize) {
                    return makeError("bson length doesn't match what we found", idElem);
                }
                frames.pop_back();
                if (frames.empty()) {
                    state = ValidationState::Done;
                } else {
                    curr = &frames.back();
                    if (curr->isCodeWithScope())
                        state = ValidationState::EndCodeWScope;
                    else
                        state = ValidationState::WithinObj;
                }
                break;
            }
            case ValidationState::BeginCodeWScope: {
                frames.push_back(ValidationObjectFrame());
                curr = &frames.back();
                curr->setStartPosition(buffer->position());
                curr->setIsCodeWithScope(true);
                if (!buffer->readNumber<int>(&curr->expectedSize))
                    return makeError("invalid bson CodeWScope size", idElem);
                Status status = buffer->readUTF8String(NULL);
                if (!status.isOK())
                    return status;
                state = ValidationState::BeginObj;
                break;
            }
            case ValidationState::EndCodeWScope: {
                int actualLength = buffer->position() - curr->startPosition();
                if (actualLength != curr->expectedSize) {
                    return makeError("bson length for CodeWScope doesn't match what we found",
                                     idElem);
                }
                frames.pop_back();
                if (frames.empty())
                    return makeError("unnested CodeWScope", idElem);
                curr = &frames.back();
                state = ValidationState::WithinObj;
                break;
            }
            case ValidationState::Done:
                break;
        }
    }

    return Status::OK();
}

}  // namespace

Status validateBSON(const char* originalBuffer, uint64_t maxLength) {
    if (maxLength < 5) {
        return Status(ErrorCodes::InvalidBSON, "bson data has to be at least 5 bytes");
    }

    Buffer buf(originalBuffer, maxLength);
    return validateBSONIterative(&buf);
}

}  // namespace mongo
