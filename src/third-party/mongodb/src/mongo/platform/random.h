// random.h

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

#include "mongo/platform/cstdint.h"

namespace mongo {

/**
 * Uses http://en.wikipedia.org/wiki/Xorshift
 */
class PseudoRandom {
public:
    PseudoRandom(int32_t seed);

    PseudoRandom(uint32_t seed);

    PseudoRandom(int64_t seed);

    int32_t nextInt32();

    int64_t nextInt64();

    /**
     * @return a number between 0 and max
     */
    int32_t nextInt32(int32_t max) {
        return nextInt32() % max;
    }

    /**
     * @return a number between 0 and max
     */
    int64_t nextInt64(int64_t max) {
        return nextInt64() % max;
    }

    /**
     * @return a number between 0 and max
     *
     * This makes PsuedoRandom instances passable as the third argument to std::random_shuffle
     */
    intptr_t operator()(intptr_t max) {
        if (sizeof(intptr_t) == 4)
            return static_cast<intptr_t>(nextInt32(static_cast<int32_t>(max)));
        return static_cast<intptr_t>(nextInt64(static_cast<int64_t>(max)));
    }

private:
    int32_t _x;
    int32_t _y;
    int32_t _z;
    int32_t _w;
};

/**
 * More secure random numbers
 * Suitable for nonce/crypto
 * Slower than PseudoRandom, so only use when really need
 */
class SecureRandom {
public:
    virtual ~SecureRandom();

    virtual int64_t nextInt64() = 0;

    static SecureRandom* create();
};
}
