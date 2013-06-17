/**
 * Copyright (c) 2012 10gen Inc.
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
 */

#pragma once

#include <algorithm>
#include "bson/bsonobj.h"
#include "bson/bsontypes.h"
#include "bson/bsonmisc.h"
#include "bson/oid.h"
#include "util/intrusive_counter.h"
#include "util/optime.h"


namespace mongo {
    class Document;
    class DocumentStorage;
    class Value;

    //TODO: a MutableVector, similar to MutableDocument
    /// A heap-allocated reference-counted std::vector
    class RCVector : public RefCountable {
    public:
        RCVector() {}
        RCVector(const vector<Value>& v) :vec(v) {}
        vector<Value> vec;
    };

    class RCCodeWScope : public RefCountable {
    public:
        RCCodeWScope(const string& str, BSONObj obj) :code(str), scope(obj.getOwned()) {}
        const string code;
        const BSONObj scope; // Not worth converting to Document for now
    };

    class RCDBRef : public RefCountable {
    public:
        RCDBRef(const string& str, const OID& o) :ns(str), oid(o) {}
        const string ns;
        const OID oid;
    };

#pragma pack(1)
    class ValueStorage {
    public:
        // Note: it is important the memory is zeroed out (by calling zero()) at the start of every
        // constructor. Much code relies on every byte being predictably initialized to zero.

        // This is a "missing" Value
        ValueStorage() { zero(); type = EOO; }

        explicit ValueStorage(BSONType t)                  { zero(); type = t; }
        ValueStorage(BSONType t, int i)                    { zero(); type = t; intValue = i; }
        ValueStorage(BSONType t, long long l)              { zero(); type = t; longValue = l; }
        ValueStorage(BSONType t, double d)                 { zero(); type = t; doubleValue = d; }
        ValueStorage(BSONType t, ReplTime r)               { zero(); type = t; timestampValue = r; }
        ValueStorage(BSONType t, bool b)                   { zero(); type = t; boolValue = b; }
        ValueStorage(BSONType t, const Document& d)        { zero(); type = t; putDocument(d); }
        ValueStorage(BSONType t, const RCVector* a)        { zero(); type = t; putVector(a); }
        ValueStorage(BSONType t, const StringData& s)      { zero(); type = t; putString(s); }
        ValueStorage(BSONType t, const BSONBinData& bd)    { zero(); type = t; putBinData(bd); }
        ValueStorage(BSONType t, const BSONRegEx& re)      { zero(); type = t; putRegEx(re); }
        ValueStorage(BSONType t, const BSONCodeWScope& cs) { zero(); type = t; putCodeWScope(cs); }
        ValueStorage(BSONType t, const BSONDBRef& dbref)   { zero(); type = t; putDBRef(dbref); }

        ValueStorage(BSONType t, const OID& o) {
            zero();
            type = t;
            memcpy(&oid, &o, sizeof(OID));
            BOOST_STATIC_ASSERT(sizeof(OID) == sizeof(oid));
        }

        ValueStorage(const ValueStorage& rhs) {
            memcpy(this, &rhs, sizeof(*this));
            memcpyed();
        }

        ~ValueStorage() {
            if (refCounter)
                intrusive_ptr_release(genericRCPtr);
            DEV memset(this, 0xee, sizeof(*this));
        }

        ValueStorage& operator= (ValueStorage rhsCopy) {
            this->swap(rhsCopy);
            return *this;
        }

        void swap(ValueStorage& rhs) {
            // Don't need to update ref-counts because they will be the same in the end
            char temp[sizeof(ValueStorage)];
            memcpy(temp, this, sizeof(*this));
            memcpy(this, &rhs, sizeof(*this));
            memcpy(&rhs, temp, sizeof(*this));
        }

        /// Call this after memcpying to update ref counts if needed
        void memcpyed() const {
            if (refCounter)
                intrusive_ptr_add_ref(genericRCPtr);
        }

        /// These are only to be called during Value construction on an empty Value
        void putString(const StringData& s);
        void putVector(const RCVector* v);
        void putDocument(const Document& d);
        void putRegEx(const BSONRegEx& re);
        void putBinData(const BSONBinData& bd) {
            putRefCountable(
                RCString::create(
                    StringData(static_cast<const char*>(bd.data), bd.length)));
            binSubType = bd.type;
        }

        void putDBRef(const BSONDBRef& dbref) {
            putRefCountable(new RCDBRef(dbref.ns.toString(), dbref.oid));
        }

        void putCodeWScope(const BSONCodeWScope& cws) {
            putRefCountable(new RCCodeWScope(cws.code.toString(), cws.scope));
        }

        void putRefCountable(intrusive_ptr<const RefCountable> ptr) {
            genericRCPtr = ptr.get();

            if (genericRCPtr) {
                intrusive_ptr_add_ref(genericRCPtr);
                refCounter = true;
            }
        }

        StringData getString() const {
            if (shortStr) {
                return StringData(shortStrStorage, shortStrSize);
            }
            else {
                dassert(typeid(*genericRCPtr) == typeid(const RCString));
                const RCString* stringPtr = static_cast<const RCString*>(genericRCPtr);
                return StringData(stringPtr->c_str(), stringPtr->size());
            }
        }

        const vector<Value>& getArray() const {
            dassert(typeid(*genericRCPtr) == typeid(const RCVector));
            const RCVector* arrayPtr = static_cast<const RCVector*>(genericRCPtr);
            return arrayPtr->vec;
        }

        intrusive_ptr<const RCCodeWScope> getCodeWScope() const {
            dassert(typeid(*genericRCPtr) == typeid(const RCCodeWScope));
            return static_cast<const RCCodeWScope*>(genericRCPtr);
        }

        intrusive_ptr<const RCDBRef> getDBRef() const {
            dassert(typeid(*genericRCPtr) == typeid(const RCDBRef));
            return static_cast<const RCDBRef*>(genericRCPtr);
        }

        // Document is incomplete here so this can't be inline
        Document getDocument() const;

        BSONType bsonType() const {
            return BSONType(type);
        }

        BinDataType binDataType() const {
            dassert(type == BinData);
            return BinDataType(binSubType);
        }

        void zero() {
            memset(this, 0, sizeof(*this));
        }

        // Byte-for-byte identical
        bool identical(const ValueStorage& other) const {
            return  (i64[0] == other.i64[0]
                  && i64[1] == other.i64[1]);
        }

        // This data is public because this should only be used by Value which would be a friend
        union {
            struct {
                // byte 1
                signed char type;

                // byte 2
                struct {
                    bool refCounter : 1; // true if we need to refCount
                    bool shortStr : 1; // true if we are using short strings
                    // reservedFlags: 6;
                };

                // bytes 3-16;
                union {
                    unsigned char oid[12];

                    struct {
                        char shortStrSize; // TODO Consider moving into flags union (4 bits)
                        char shortStrStorage[16/*total bytes*/ - 3/*offset*/ - 1/*NUL byte*/];
                        union {
                            char nulTerminator;
                        };
                    };

                    struct {
                        union {
                            unsigned char binSubType;
                            char pad[6];
                            char stringCache[6]; // TODO copy first few bytes of strings in here
                        };
                        union { // 8 bytes long and 8-byte aligned
                            // There should be no pointers to non-const data
                            const RefCountable* genericRCPtr;

                            double doubleValue;
                            bool boolValue;
                            int intValue;
                            long long longValue;
                            ReplTime timestampValue;
                            long long dateValue;
                        };
                    };
                };
            };

            // covers the whole ValueStorage
            long long i64[2];
        };
    };
    BOOST_STATIC_ASSERT(sizeof(ValueStorage) == 16);
#pragma pack()

}
