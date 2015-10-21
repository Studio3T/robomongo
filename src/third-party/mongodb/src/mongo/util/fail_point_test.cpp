/**
 *    Copyright (C) 2012 10gen Inc.
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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kDefault

#include "mongo/platform/basic.h"

#include <boost/thread/thread.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "mongo/stdx/functional.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/fail_point.h"
#include "mongo/util/log.h"
#include "mongo/util/time_support.h"

using mongo::FailPoint;

namespace mongo_test {
TEST(FailPoint, InitialState) {
    FailPoint failPoint;
    ASSERT_FALSE(failPoint.shouldFail());
}

TEST(FailPoint, AlwaysOn) {
    FailPoint failPoint;
    failPoint.setMode(FailPoint::alwaysOn);
    ASSERT(failPoint.shouldFail());

    MONGO_FAIL_POINT_BLOCK(failPoint, scopedFp) {
        ASSERT(scopedFp.getData().isEmpty());
    }

    for (size_t x = 0; x < 50; x++) {
        ASSERT(failPoint.shouldFail());
    }
}

TEST(FailPoint, NTimes) {
    FailPoint failPoint;
    failPoint.setMode(FailPoint::nTimes, 4);
    ASSERT(failPoint.shouldFail());
    ASSERT(failPoint.shouldFail());
    ASSERT(failPoint.shouldFail());
    ASSERT(failPoint.shouldFail());

    for (size_t x = 0; x < 50; x++) {
        ASSERT_FALSE(failPoint.shouldFail());
    }
}

TEST(FailPoint, BlockOff) {
    FailPoint failPoint;
    bool called = false;

    MONGO_FAIL_POINT_BLOCK(failPoint, scopedFp) {
        called = true;
    }

    ASSERT_FALSE(called);
}

TEST(FailPoint, BlockAlwaysOn) {
    FailPoint failPoint;
    failPoint.setMode(FailPoint::alwaysOn);
    bool called = false;

    MONGO_FAIL_POINT_BLOCK(failPoint, scopedFp) {
        called = true;
    }

    ASSERT(called);
}

TEST(FailPoint, BlockNTimes) {
    FailPoint failPoint;
    failPoint.setMode(FailPoint::nTimes, 1);
    size_t counter = 0;

    for (size_t x = 0; x < 10; x++) {
        MONGO_FAIL_POINT_BLOCK(failPoint, scopedFp) {
            counter++;
        }
    }

    ASSERT_EQUALS(1U, counter);
}

TEST(FailPoint, BlockWithException) {
    FailPoint failPoint;
    failPoint.setMode(FailPoint::alwaysOn);
    bool threw = false;

    try {
        MONGO_FAIL_POINT_BLOCK(failPoint, scopedFp) {
            throw std::logic_error("BlockWithException threw");
        }
    } catch (const std::logic_error&) {
        threw = true;
    }

    ASSERT(threw);
    // This will get into an infinite loop if reference counter was not
    // properly decremented
    failPoint.setMode(FailPoint::off);
}

TEST(FailPoint, SetGetParam) {
    FailPoint failPoint;
    failPoint.setMode(FailPoint::alwaysOn, 0, BSON("x" << 20));

    MONGO_FAIL_POINT_BLOCK(failPoint, scopedFp) {
        ASSERT_EQUALS(20, scopedFp.getData()["x"].numberInt());
    }
}

TEST(FailPoint, SetInvalidMode) {
    FailPoint failPoint;

    ASSERT_THROWS(failPoint.setMode(static_cast<FailPoint::Mode>(9999)), mongo::UserException);
    ASSERT_FALSE(failPoint.shouldFail());

    ASSERT_THROWS(failPoint.setMode(static_cast<FailPoint::Mode>(-1)), mongo::UserException);
    ASSERT_FALSE(failPoint.shouldFail());
}

class FailPointStress : public mongo::unittest::Test {
public:
    FailPointStress() : _tasks(NULL) {}

    void setUp() {
        _fp.setMode(FailPoint::alwaysOn, 0, BSON("a" << 44));
    }

    void tearDown() {
        // Note: This can loop indefinitely if reference counter was off
        _fp.setMode(FailPoint::off, 0, BSON("a" << 66));
    }

    void startTest() {
        verify(_tasks == NULL);

        _tasks = new boost::thread_group();
        _tasks->add_thread(new boost::thread(blockTask, &_fp));
        _tasks->add_thread(new boost::thread(blockWithExceptionTask, &_fp));
        _tasks->add_thread(new boost::thread(simpleTask, &_fp));
        _tasks->add_thread(new boost::thread(flipTask, &_fp));
    }

    void stopTest() {
        _tasks->interrupt_all();
        _tasks->join_all();
        delete _tasks;
        _tasks = NULL;
    }

private:
    static void blockTask(FailPoint* failPoint) {
        while (true) {
            MONGO_FAIL_POINT_BLOCK((*failPoint), scopedFp) {
                const mongo::BSONObj& data = scopedFp.getData();

                // Expanded ASSERT_EQUALS since the error is not being
                // printed out properly
                if (data["a"].numberInt() != 44) {
                    mongo::error() << "blockTask thread detected anomaly"
                                   << " - data: " << data << std::endl;
                    ASSERT(false);
                }
            }

            boost::this_thread::interruption_point();
        }
    }

    static void blockWithExceptionTask(FailPoint* failPoint) {
        while (true) {
            try {
                MONGO_FAIL_POINT_BLOCK((*failPoint), scopedFp) {
                    const mongo::BSONObj& data = scopedFp.getData();

                    if (data["a"].numberInt() != 44) {
                        mongo::error() << "blockWithExceptionTask thread detected anomaly"
                                       << " - data: " << data << std::endl;
                        ASSERT(false);
                    }

                    throw std::logic_error("blockWithExceptionTask threw");
                }
            } catch (const std::logic_error&) {
            }

            boost::this_thread::interruption_point();
        }
    }

    static void simpleTask(FailPoint* failPoint) {
        while (true) {
            static_cast<void>(MONGO_FAIL_POINT((*failPoint)));
            boost::this_thread::interruption_point();
        }
    }

    static void flipTask(FailPoint* failPoint) {
        while (true) {
            if (failPoint->shouldFail()) {
                failPoint->setMode(FailPoint::off, 0);
            } else {
                failPoint->setMode(FailPoint::alwaysOn, 0, BSON("a" << 44));
            }

            boost::this_thread::interruption_point();
        }
    }

    FailPoint _fp;
    boost::thread_group* _tasks;
};

TEST_F(FailPointStress, Basic) {
    startTest();
    mongo::sleepsecs(30);
    stopTest();
}

static void parallelFailPointTestThread(FailPoint* fp,
                                        const int64_t numIterations,
                                        const int32_t seed,
                                        int64_t* outNumActivations) {
    fp->setThreadPRNGSeed(seed);
    int64_t numActivations = 0;
    for (int64_t i = 0; i < numIterations; ++i) {
        if (fp->shouldFail()) {
            ++numActivations;
        }
    }
    *outNumActivations = numActivations;
}
/**
 * Encounters a failpoint with the given fpMode and fpVal numEncountersPerThread
 * times in each of numThreads parallel threads, and returns the number of total
 * times that the failpoint was activiated.
 */
static int64_t runParallelFailPointTest(FailPoint::Mode fpMode,
                                        FailPoint::ValType fpVal,
                                        const int32_t numThreads,
                                        const int32_t numEncountersPerThread) {
    ASSERT_GT(numThreads, 0);
    ASSERT_GT(numEncountersPerThread, 0);
    FailPoint failPoint;
    failPoint.setMode(fpMode, fpVal);
    std::vector<boost::thread*> tasks;
    std::vector<int64_t> counts(numThreads, 0);
    ASSERT_EQUALS(static_cast<uint32_t>(numThreads), counts.size());
    for (int32_t i = 0; i < numThreads; ++i) {
        tasks.push_back(new boost::thread(parallelFailPointTestThread,
                                          &failPoint,
                                          numEncountersPerThread,
                                          i,  // hardcoded seed, different for each thread.
                                          &counts[i]));
    }
    int64_t totalActivations = 0;
    for (int32_t i = 0; i < numThreads; ++i) {
        tasks[i]->join();
        delete tasks[i];
        totalActivations += counts[i];
    }
    return totalActivations;
}

TEST(FailPoint, RandomActivationP0) {
    ASSERT_EQUALS(0, runParallelFailPointTest(FailPoint::random, 0, 1, 1000000));
}

TEST(FailPoint, RandomActivationP5) {
    ASSERT_APPROX_EQUAL(500000,
                        runParallelFailPointTest(
                            FailPoint::random, std::numeric_limits<int32_t>::max() / 2, 10, 100000),
                        500);
}

TEST(FailPoint, RandomActivationP01) {
    ASSERT_APPROX_EQUAL(
        10000,
        runParallelFailPointTest(
            FailPoint::random, std::numeric_limits<int32_t>::max() / 100, 10, 100000),
        500);
}

TEST(FailPoint, RandomActivationP001) {
    ASSERT_APPROX_EQUAL(
        1000,
        runParallelFailPointTest(
            FailPoint::random, std::numeric_limits<int32_t>::max() / 1000, 10, 100000),
        500);
}
}
