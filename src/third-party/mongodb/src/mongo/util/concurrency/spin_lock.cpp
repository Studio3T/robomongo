// spin_lock.cpp

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

#include "mongo/platform/basic.h"
#undef MONGO_PCH_WHITELISTED  // todo eliminate this include

#include "mongo/util/concurrency/spin_lock.h"

#include <time.h>

#include "mongo/bson/inline_decls.h"

namespace mongo {

SpinLock::~SpinLock() {
#if defined(_WIN32)
    DeleteCriticalSection(&_cs);
#elif defined(__USE_XOPEN2K)
    pthread_spin_destroy(&_lock);
#endif
}

SpinLock::SpinLock()
#if defined(_WIN32)
{
    InitializeCriticalSectionAndSpinCount(&_cs, 4000);
}
#elif defined(__USE_XOPEN2K)
{
    pthread_spin_init(&_lock, 0);
}
#elif defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
    : _locked(false) {
}
#else
    : _mutex("SpinLock") {
}
#endif

#if defined(__USE_XOPEN2K)
NOINLINE_DECL void SpinLock::_lk() {
    /**
     * this is designed to perform close to the default spin lock
     * the reason for the mild insanity is to prevent horrible performance
     * when contention spikes
     * it allows spinlocks to be used in many more places
     * which is good because even with this change they are about 8x faster on linux
     */

    for (int i = 0; i < 1000; i++) {
        if (pthread_spin_trylock(&_lock) == 0)
            return;
#if defined(__i386__) || defined(__x86_64__)
        asm volatile("pause");  // maybe trylock does this; just in case.
#endif
    }

    for (int i = 0; i < 1000; i++) {
        if (pthread_spin_trylock(&_lock) == 0)
            return;
        pthread_yield();
    }

    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = 5000000;

    while (pthread_spin_trylock(&_lock) != 0) {
        nanosleep(&t, NULL);
    }
}
#elif defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
void SpinLock::lock() {
    // fast path
    if (!_locked && !__sync_lock_test_and_set(&_locked, true)) {
        return;
    }

    // wait for lock
    int wait = 1000;
    while ((wait-- > 0) && (_locked)) {
#if defined(__i386__) || defined(__x86_64__)
        asm volatile("pause");
#endif
    }

    // if failed to grab lock, sleep
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = 5000000;
    while (__sync_lock_test_and_set(&_locked, true)) {
        nanosleep(&t, NULL);
    }
}
#endif

bool SpinLock::isfast() {
#if defined(_WIN32) || defined(__USE_XOPEN2K) || defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
    return true;
#else
    return false;
#endif
}


}  // namespace mongo
