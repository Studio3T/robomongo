/**
 *    Copyright (C) 2009-2014 MongoDB Inc.
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
 *    must comply with the GNU Affero General Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kDefault

#include "mongo/platform/basic.h"

#include "mongo/db/storage/mmap_v1/aligned_builder.h"

#include "mongo/util/debug_util.h"
#include "mongo/util/log.h"

namespace mongo {

using std::endl;

AlignedBuilder::AlignedBuilder(unsigned initSize) {
    _len = 0;
    _malloc(initSize);
    uassert(13584, "out of memory AlignedBuilder", _p._allocationAddress);
}

BOOST_STATIC_ASSERT(sizeof(void*) == sizeof(size_t));

/** reset for a re-use. shrinks if > 128MB */
void AlignedBuilder::reset() {
    _len = 0;
    RARELY {
        const unsigned sizeCap = 128 * 1024 * 1024;
        if (_p._size > sizeCap)
            _realloc(sizeCap, _len);
    }
}

/** reset with a hint as to the upcoming needed size specified */
void AlignedBuilder::reset(unsigned sz) {
    _len = 0;
    unsigned Q = 32 * 1024 * 1024 - 1;
    unsigned want = (sz + Q) & (~Q);
    if (_p._size == want) {
        return;
    }
    if (_p._size > want) {
        if (_p._size <= 64 * 1024 * 1024)
            return;
        bool downsize = false;
        RARELY {
            downsize = true;
        }
        if (!downsize)
            return;
    }
    _realloc(want, _len);
}

void AlignedBuilder::mallocSelfAligned(unsigned sz) {
    verify(sz == _p._size);
    void* p = malloc(sz + Alignment - 1);
    _p._allocationAddress = p;
    size_t s = (size_t)p;
    size_t sold = s;
    s += Alignment - 1;
    s = (s / Alignment) * Alignment;
    verify(s >= sold);                                // beginning
    verify((s + sz) <= (sold + sz + Alignment - 1));  // end
    _p._data = (char*)s;
}

/* "slow"/infrequent portion of 'grow()'  */
void NOINLINE_DECL AlignedBuilder::growReallocate(unsigned oldLen) {
    const unsigned MB = 1024 * 1024;
    const unsigned kMaxSize = (sizeof(int*) == 4) ? 512 * MB : 2000 * MB;
    const unsigned kWarnSize = (sizeof(int*) == 4) ? 256 * MB : 512 * MB;

    const unsigned oldSize = _p._size;

    // Warn for unexpectedly large buffer
    wassert(_len <= kWarnSize);

    // Check validity of requested size
    invariant(_len > oldSize);
    if (_len > kMaxSize) {
        log() << "error writing journal: too much uncommitted data (" << _len << " bytes)";
        log() << "shutting down immediately to avoid corruption";
        fassert(28614, _len <= kMaxSize);
    }

    // Use smaller maximum for debug builds, as we should never be close the the maximum
    dassert(_len <= 256 * MB);

    // Compute newSize by doubling the existing maximum size until the maximum is reached
    invariant(oldSize > 0);
    uint64_t newSize = oldSize;  // use 64 bits to defend against accidental overflow
    while (newSize < _len) {
        newSize *= 2;
    }

    if (newSize > kMaxSize) {
        newSize = kMaxSize;
    }

    _realloc(newSize, oldLen);
}

void AlignedBuilder::_malloc(unsigned sz) {
    _p._size = sz;
#if defined(_WIN32)
    void* p = VirtualAlloc(0, sz, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    _p._allocationAddress = p;
    _p._data = (char*)p;
#elif defined(__linux__)
    // in theory #ifdef _POSIX_VERSION should work, but it doesn't on OS X 10.4, and needs to be
    // tested on solaris. so for now, linux only for this.
    void* p = 0;
    int res = posix_memalign(&p, Alignment, sz);
    massert(13524, "out of memory AlignedBuilder", res == 0);
    _p._allocationAddress = p;
    _p._data = (char*)p;
#else
    mallocSelfAligned(sz);
    verify(((size_t)_p._data) % Alignment == 0);
#endif
}

void AlignedBuilder::_realloc(unsigned newSize, unsigned oldLen) {
    // posix_memalign alignment is not maintained on reallocs, so we can't use realloc().
    AllocationInfo old = _p;
    _malloc(newSize);
    verify(oldLen <= _len);
    memcpy(_p._data, old._data, oldLen);
    _free(old._allocationAddress);
}

void AlignedBuilder::_free(void* p) {
#if defined(_WIN32)
    VirtualFree(p, 0, MEM_RELEASE);
#else
    free(p);
#endif
}

void AlignedBuilder::kill() {
    _free(_p._allocationAddress);
    _p._allocationAddress = 0;
    _p._data = 0;
}
}
