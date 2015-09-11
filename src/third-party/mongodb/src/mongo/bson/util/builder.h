/* builder.h */

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

#include <cfloat>
#include <sstream>
#include <stdio.h>
#include <string>
#include <string.h>

#include <boost/static_assert.hpp>

#include "mongo/base/data_view.h"
#include "mongo/base/string_data.h"
#include "mongo/bson/inline_decls.h"
#include "mongo/util/allocator.h"
#include "mongo/util/assert_util.h"

namespace mongo {
/* Accessing unaligned doubles on ARM generates an alignment trap and aborts with SIGBUS on Linux.
   Wrapping the double in a packed struct forces gcc to generate code that works with unaligned
   values too. The generated code for other architectures (which already allow unaligned accesses)
   is the same as if there was a direct pointer access.
*/
struct PackedDouble {
    double d;
} PACKED_DECL;


/* Note the limit here is rather arbitrary and is simply a standard. generally the code works
   with any object that fits in ram.

   Also note that the server has some basic checks to enforce this limit but those checks are not
   exhaustive for example need to check for size too big after
     update $push (append) operation
     various db.eval() type operations
*/
const int BSONObjMaxUserSize = 16 * 1024 * 1024;

/*
   Sometimes we need objects slightly larger - an object in the replication local.oplog
   is slightly larger than a user object for example.
*/
const int BSONObjMaxInternalSize = BSONObjMaxUserSize + (16 * 1024);

const int BufferMaxSize = 64 * 1024 * 1024;

template <typename Allocator>
class StringBuilderImpl;

class TrivialAllocator {
public:
    void* Malloc(size_t sz) {
        return mongoMalloc(sz);
    }
    void* Realloc(void* p, size_t sz) {
        return mongoRealloc(p, sz);
    }
    void Free(void* p) {
        free(p);
    }
};

class StackAllocator {
public:
    enum { SZ = 512 };
    void* Malloc(size_t sz) {
        if (sz <= SZ)
            return buf;
        return mongoMalloc(sz);
    }
    void* Realloc(void* p, size_t sz) {
        if (p == buf) {
            if (sz <= SZ)
                return buf;
            void* d = mongoMalloc(sz);
            if (d == 0)
                msgasserted(15912, "out of memory StackAllocator::Realloc");
            memcpy(d, p, SZ);
            return d;
        }
        return mongoRealloc(p, sz);
    }
    void Free(void* p) {
        if (p != buf)
            free(p);
    }

private:
    char buf[SZ];
};

template <class Allocator>
class _BufBuilder {
    // non-copyable, non-assignable
    _BufBuilder(const _BufBuilder&);
    _BufBuilder& operator=(const _BufBuilder&);
    Allocator al;

public:
    _BufBuilder(int initsize = 512) : size(initsize) {
        if (size > 0) {
            data = (char*)al.Malloc(size);
            if (data == 0)
                msgasserted(10000, "out of memory BufBuilder");
        } else {
            data = 0;
        }
        l = 0;
        reservedBytes = 0;
    }
    ~_BufBuilder() {
        kill();
    }

    void kill() {
        if (data) {
            al.Free(data);
            data = 0;
        }
    }

    void reset() {
        l = 0;
        reservedBytes = 0;
    }
    void reset(int maxSize) {
        l = 0;
        reservedBytes = 0;
        if (maxSize && size > maxSize) {
            al.Free(data);
            data = (char*)al.Malloc(maxSize);
            if (data == 0)
                msgasserted(15913, "out of memory BufBuilder::reset");
            size = maxSize;
        }
    }

    /** leave room for some stuff later
        @return point to region that was skipped.  pointer may change later (on realloc), so for
        immediate use only
    */
    char* skip(int n) {
        return grow(n);
    }

    /* note this may be deallocated (realloced) if you keep writing. */
    char* buf() {
        return data;
    }
    const char* buf() const {
        return data;
    }

    /* assume ownership of the buffer - you must then free() it */
    void decouple() {
        data = 0;
    }

    void appendUChar(unsigned char j) {
        BOOST_STATIC_ASSERT(CHAR_BIT == 8);
        appendNumImpl(j);
    }
    void appendChar(char j) {
        appendNumImpl(j);
    }
    void appendNum(char j) {
        appendNumImpl(j);
    }
    void appendNum(short j) {
        BOOST_STATIC_ASSERT(sizeof(short) == 2);
        appendNumImpl(j);
    }
    void appendNum(int j) {
        BOOST_STATIC_ASSERT(sizeof(int) == 4);
        appendNumImpl(j);
    }
    void appendNum(unsigned j) {
        appendNumImpl(j);
    }

// Bool does not have a well defined encoding.
#if __cplusplus >= 201103L
    void appendNum(bool j) = delete;
#else
    void appendNum(bool j) {
        invariant(false);
    }
#endif

    void appendNum(double j) {
        BOOST_STATIC_ASSERT(sizeof(double) == 8);
        appendNumImpl(j);
    }
    void appendNum(long long j) {
        BOOST_STATIC_ASSERT(sizeof(long long) == 8);
        appendNumImpl(j);
    }
    void appendNum(unsigned long long j) {
        appendNumImpl(j);
    }

    void appendBuf(const void* src, size_t len) {
        memcpy(grow((int)len), src, len);
    }

    template <class T>
    void appendStruct(const T& s) {
        appendBuf(&s, sizeof(T));
    }

    void appendStr(const StringData& str, bool includeEndingNull = true) {
        const int len = str.size() + (includeEndingNull ? 1 : 0);
        str.copyTo(grow(len), includeEndingNull);
    }

    /** @return length of current std::string */
    int len() const {
        return l;
    }
    void setlen(int newLen) {
        l = newLen;
    }
    /** @return size of the buffer */
    int getSize() const {
        return size;
    }

    /* returns the pre-grow write position */
    inline char* grow(int by) {
        int oldlen = l;
        int newLen = l + by;
        int minSize = newLen + reservedBytes;
        if (minSize > size) {
            grow_reallocate(minSize);
        }
        l = newLen;
        return data + oldlen;
    }

    /**
     * Reserve room for some number of bytes to be claimed at a later time.
     */
    void reserveBytes(int bytes) {
        int minSize = l + reservedBytes + bytes;
        if (minSize > size)
            grow_reallocate(minSize);

        // This must happen *after* any attempt to grow.
        reservedBytes += bytes;
    }

    /**
     * Claim an earlier reservation of some number of bytes. These bytes must already have been
     * reserved. Appends of up to this many bytes immediately following a claim are
     * guaranteed to succeed without a need to reallocate.
     */
    void claimReservedBytes(int bytes) {
        invariant(reservedBytes >= bytes);
        reservedBytes -= bytes;
    }

private:
    template <typename T>
    void appendNumImpl(T t) {
        // NOTE: For now, we assume that all things written
        // by a BufBuilder are intended for external use: either written to disk
        // or to the wire. Since all of our encoding formats are little endian,
        // we bake that assumption in here. This decision should be revisited soon.
        DataView(grow(sizeof(t))).writeLE(t);
    }


    /* "slow" portion of 'grow()'  */
    void NOINLINE_DECL grow_reallocate(int minSize) {
        int a = 64;
        while (a < minSize)
            a = a * 2;

        if (a > BufferMaxSize) {
            std::stringstream ss;
            ss << "BufBuilder attempted to grow() to " << a << " bytes, past the 64MB limit.";
            msgasserted(13548, ss.str().c_str());
        }
        data = (char*)al.Realloc(data, a);
        if (data == NULL)
            msgasserted(16070, "out of memory BufBuilder::grow_reallocate");
        size = a;
    }

    char* data;
    int l;
    int size;
    int reservedBytes;  // eagerly grow_reallocate to keep this many bytes of spare room.

    friend class StringBuilderImpl<Allocator>;
};

typedef _BufBuilder<TrivialAllocator> BufBuilder;

/** The StackBufBuilder builds smaller datasets on the stack instead of using malloc.
      this can be significantly faster for small bufs.  However, you can not decouple() the
      buffer with StackBufBuilder.
    While designed to be a variable on the stack, if you were to dynamically allocate one,
      nothing bad would happen.  In fact in some circumstances this might make sense, say,
      embedded in some other object.
*/
class StackBufBuilder : public _BufBuilder<StackAllocator> {
public:
    StackBufBuilder() : _BufBuilder<StackAllocator>(StackAllocator::SZ) {}
    void decouple();  // not allowed. not implemented.
};

#if defined(_WIN32)
#pragma push_macro("snprintf")
#define snprintf _snprintf
#endif

/** std::stringstream deals with locale so this is a lot faster than std::stringstream for UTF8 */
template <typename Allocator>
class StringBuilderImpl {
public:
    // Sizes are determined based on the number of characters in 64-bit + the trailing '\0'
    static const size_t MONGO_DBL_SIZE = 3 + DBL_MANT_DIG - DBL_MIN_EXP + 1;
    static const size_t MONGO_S32_SIZE = 12;
    static const size_t MONGO_U32_SIZE = 11;
    static const size_t MONGO_S64_SIZE = 23;
    static const size_t MONGO_U64_SIZE = 22;
    static const size_t MONGO_S16_SIZE = 7;
    static const size_t MONGO_PTR_SIZE = 19;  // Accounts for the 0x prefix

    StringBuilderImpl() {}

    StringBuilderImpl& operator<<(double x) {
        return SBNUM(x, MONGO_DBL_SIZE, "%g");
    }
    StringBuilderImpl& operator<<(int x) {
        return SBNUM(x, MONGO_S32_SIZE, "%d");
    }
    StringBuilderImpl& operator<<(unsigned x) {
        return SBNUM(x, MONGO_U32_SIZE, "%u");
    }
    StringBuilderImpl& operator<<(long x) {
        return SBNUM(x, MONGO_S64_SIZE, "%ld");
    }
    StringBuilderImpl& operator<<(unsigned long x) {
        return SBNUM(x, MONGO_U64_SIZE, "%lu");
    }
    StringBuilderImpl& operator<<(long long x) {
        return SBNUM(x, MONGO_S64_SIZE, "%lld");
    }
    StringBuilderImpl& operator<<(unsigned long long x) {
        return SBNUM(x, MONGO_U64_SIZE, "%llu");
    }
    StringBuilderImpl& operator<<(short x) {
        return SBNUM(x, MONGO_S16_SIZE, "%hd");
    }
    StringBuilderImpl& operator<<(const void* x) {
        if (sizeof(x) == 8) {
            return SBNUM(x, MONGO_PTR_SIZE, "0x%llX");
        } else {
            return SBNUM(x, MONGO_PTR_SIZE, "0x%lX");
        }
    }
    StringBuilderImpl& operator<<(char c) {
        _buf.grow(1)[0] = c;
        return *this;
    }
    StringBuilderImpl& operator<<(const char* str) {
        return *this << StringData(str);
    }
    StringBuilderImpl& operator<<(const StringData& str) {
        append(str);
        return *this;
    }

    void appendDoubleNice(double x) {
        const int prev = _buf.l;
        const int maxSize = 32;
        char* start = _buf.grow(maxSize);
        int z = snprintf(start, maxSize, "%.16g", x);
        verify(z >= 0);
        verify(z < maxSize);
        _buf.l = prev + z;
        if (strchr(start, '.') == 0 && strchr(start, 'E') == 0 && strchr(start, 'N') == 0) {
            write(".0", 2);
        }
    }

    void write(const char* buf, int len) {
        memcpy(_buf.grow(len), buf, len);
    }

    void append(const StringData& str) {
        str.copyTo(_buf.grow(str.size()), false);
    }

    void reset(int maxSize = 0) {
        _buf.reset(maxSize);
    }

    std::string str() const {
        return std::string(_buf.data, _buf.l);
    }

    /** size of current std::string */
    int len() const {
        return _buf.l;
    }

private:
    _BufBuilder<Allocator> _buf;

    // non-copyable, non-assignable
    StringBuilderImpl(const StringBuilderImpl&);
    StringBuilderImpl& operator=(const StringBuilderImpl&);

    template <typename T>
    StringBuilderImpl& SBNUM(T val, int maxSize, const char* macro) {
        int prev = _buf.l;
        int z = snprintf(_buf.grow(maxSize), maxSize, macro, (val));
        verify(z >= 0);
        verify(z < maxSize);
        _buf.l = prev + z;
        return *this;
    }
};

typedef StringBuilderImpl<TrivialAllocator> StringBuilder;
typedef StringBuilderImpl<StackAllocator> StackStringBuilder;

#if defined(_WIN32)
#undef snprintf
#pragma pop_macro("snprintf")
#endif
}  // namespace mongo
