/*    Copyright 2010 10gen Inc.
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

#include "mongo/platform/basic.h"
#include "mongo/platform/cstdint.h"
#include "mongo/util/time_support.h"

#include <cstdio>
#include <string>
#include <iostream>
#include <boost/thread/thread.hpp>
#include <boost/thread/tss.hpp>
#include <boost/thread/xtime.hpp>

#include "mongo/util/assert_util.h"

#ifdef _WIN32
#include <boost/date_time/filetime_functions.hpp>
#include "mongo/util/concurrency/mutex.h"
#include "mongo/util/timer.h"
#endif

namespace mongo {

    // jsTime_virtual_skew is just for testing. a test command manipulates it.
    long long jsTime_virtual_skew = 0;
    boost::thread_specific_ptr<long long> jsTime_virtual_thread_skew;

    using std::string;

    void time_t_to_Struct(time_t t, struct tm * buf , bool local) {
#if defined(_WIN32)
        if ( local )
            localtime_s( buf , &t );
        else
            gmtime_s(buf, &t);
#else
        if ( local )
            localtime_r(&t, buf);
        else
            gmtime_r(&t, buf);
#endif
    }

#if defined(_WIN32)
    void curTimeString(char* timeStr) {
        boost::xtime xt;
        boost::xtime_get(&xt, MONGO_BOOST_TIME_UTC);
        time_t_to_String(xt.sec, timeStr);

        char* milliSecStr = timeStr + 19;
        _snprintf(milliSecStr, 5, ".%03d", static_cast<int32_t>(xt.nsec / 1000000));
    }
#else
    void curTimeString(char* timeStr) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        time_t_to_String(tv.tv_sec, timeStr);

        char* milliSecStr = timeStr + 19;
        snprintf(milliSecStr, 5, ".%03d", static_cast<int32_t>(tv.tv_usec / 1000));
    }
#endif

    // uses ISO 8601 dates without trailing Z
    // colonsOk should be false when creating filenames
    string terseCurrentTime(bool colonsOk) {
        struct tm t;
        time_t_to_Struct( time(0) , &t );

        const char* fmt = (colonsOk ? "%Y-%m-%dT%H:%M:%S" : "%Y-%m-%dT%H-%M-%S");
        char buf[32];
        fassert(16226, strftime(buf, sizeof(buf), fmt, &t) == 19);
        return buf;
    }

    string timeToISOString(time_t time) {
        struct tm t;
        time_t_to_Struct( time, &t );

        const char* fmt = "%Y-%m-%dT%H:%M:%SZ";
        char buf[32];
        fassert(16227, strftime(buf, sizeof(buf), fmt, &t) == 20);
        return buf;
    }

    boost::gregorian::date currentDate() {
        boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
        return now.date();
    }

    // parses time of day in "hh:mm" format assuming 'hh' is 00-23
    bool toPointInTime( const string& str , boost::posix_time::ptime* timeOfDay ) {
        int hh = 0;
        int mm = 0;
        if ( 2 != sscanf( str.c_str() , "%d:%d" , &hh , &mm ) ) {
            return false;
        }

        // verify that time is well formed
        if ( ( hh / 24 ) || ( mm / 60 ) ) {
            return false;
        }

        boost::posix_time::ptime res( currentDate() , boost::posix_time::hours( hh ) + boost::posix_time::minutes( mm ) );
        *timeOfDay = res;
        return true;
    }

#if defined(_WIN32)
    void sleepsecs(int s) {
        Sleep(s*1000);
    }
    void sleepmillis(long long s) {
        fassert(16228, s <= 0xffffffff );
        Sleep((DWORD) s);
    }
    void sleepmicros(long long s) {
        if ( s <= 0 )
            return;
        boost::xtime xt;
        boost::xtime_get(&xt, MONGO_BOOST_TIME_UTC);
        xt.sec += (int)( s / 1000000 );
        xt.nsec += (int)(( s % 1000000 ) * 1000);
        if ( xt.nsec >= 1000000000 ) {
            xt.nsec -= 1000000000;
            xt.sec++;
        }
        boost::thread::sleep(xt);
    }
#elif defined(__sunos__)
    void sleepsecs(int s) {
        boost::xtime xt;
        boost::xtime_get(&xt, MONGO_BOOST_TIME_UTC);
        xt.sec += s;
        boost::thread::sleep(xt);
    }
    void sleepmillis(long long s) {
        boost::xtime xt;
        boost::xtime_get(&xt, MONGO_BOOST_TIME_UTC);
        xt.sec += (int)( s / 1000 );
        xt.nsec += (int)(( s % 1000 ) * 1000000);
        if ( xt.nsec >= 1000000000 ) {
            xt.nsec -= 1000000000;
            xt.sec++;
        }
        boost::thread::sleep(xt);
    }
    void sleepmicros(long long s) {
        if ( s <= 0 )
            return;
        boost::xtime xt;
        boost::xtime_get(&xt, MONGO_BOOST_TIME_UTC);
        xt.sec += (int)( s / 1000000 );
        xt.nsec += (int)(( s % 1000000 ) * 1000);
        if ( xt.nsec >= 1000000000 ) {
            xt.nsec -= 1000000000;
            xt.sec++;
        }
        boost::thread::sleep(xt);
    }
#else
    void sleepsecs(int s) {
        struct timespec t;
        t.tv_sec = s;
        t.tv_nsec = 0;
        if ( nanosleep( &t , 0 ) ) {
            std::cout << "nanosleep failed" << std::endl;
        }
    }
    void sleepmicros(long long s) {
        if ( s <= 0 )
            return;
        struct timespec t;
        t.tv_sec = (int)(s / 1000000);
        t.tv_nsec = 1000 * ( s % 1000000 );
        struct timespec out;
        if ( nanosleep( &t , &out ) ) {
            std::cout << "nanosleep failed" << std::endl;
        }
    }
    void sleepmillis(long long s) {
        sleepmicros( s * 1000 );
    }
#endif

    void Backoff::nextSleepMillis(){

        // Get the current time
        unsigned long long currTimeMillis = curTimeMillis64();

        int lastSleepMillis = _lastSleepMillis;

        if( _lastErrorTimeMillis == 0 || _lastErrorTimeMillis > currTimeMillis /* VM bugs exist */ )
            _lastErrorTimeMillis = currTimeMillis;
        unsigned long long lastErrorTimeMillis = _lastErrorTimeMillis;
        _lastErrorTimeMillis = currTimeMillis;

        // Backoff logic

        // Get the time since the last error
        unsigned long long timeSinceLastErrorMillis = currTimeMillis - lastErrorTimeMillis;

        // Makes the cast below safe
        verify( _resetAfterMillis >= 0 );

        // If we haven't seen another error recently (3x the max wait time), reset our
        // wait counter.
        if( timeSinceLastErrorMillis > (unsigned)( _resetAfterMillis ) ) lastSleepMillis = 0;

        // Makes the test below sane
        verify( _maxSleepMillis > 0 );

        // Wait a power of two millis
        if( lastSleepMillis == 0 ) lastSleepMillis = 1;
        else lastSleepMillis = std::min( lastSleepMillis * 2, _maxSleepMillis );

        // Store the last slept time
        _lastSleepMillis = lastSleepMillis;
        sleepmillis( lastSleepMillis );
    }

    extern long long jsTime_virtual_skew;
    extern boost::thread_specific_ptr<long long> jsTime_virtual_thread_skew;

    // DO NOT TOUCH except for testing
    void jsTimeVirtualSkew( long long skew ){
        jsTime_virtual_skew = skew;
    }
    long long getJSTimeVirtualSkew(){
        return jsTime_virtual_skew;
    }

    void jsTimeVirtualThreadSkew( long long skew ){
        jsTime_virtual_thread_skew.reset(new long long(skew));
    }
    long long getJSTimeVirtualThreadSkew(){
        if(jsTime_virtual_thread_skew.get()){
            return *(jsTime_virtual_thread_skew.get());
        }
        else return 0;
    }

    /** Date_t is milliseconds since epoch */
    Date_t jsTime();

    /** warning this will wrap */
    unsigned curTimeMicros();

    unsigned long long curTimeMicros64();
#ifdef _WIN32 // no gettimeofday on windows
    unsigned long long curTimeMillis64() {
        boost::xtime xt;
        boost::xtime_get(&xt, MONGO_BOOST_TIME_UTC);
        return ((unsigned long long)xt.sec) * 1000 + xt.nsec / 1000000;
    }
    Date_t jsTime() {
        boost::xtime xt;
        boost::xtime_get(&xt, MONGO_BOOST_TIME_UTC);
        unsigned long long t = xt.nsec / 1000000;
        return ((unsigned long long) xt.sec * 1000) + t + getJSTimeVirtualSkew() + getJSTimeVirtualThreadSkew();
    }

    static unsigned long long getFiletime() {
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        return *reinterpret_cast<unsigned long long*>(&ft);
    }

    static unsigned long long getPerfCounter() {
        LARGE_INTEGER li;
        QueryPerformanceCounter(&li);
        return li.QuadPart;
    }

    static unsigned long long baseFiletime = 0;
    static unsigned long long basePerfCounter = 0;
    static unsigned long long resyncInterval = 0;
    static SimpleMutex _curTimeMicros64ReadMutex("curTimeMicros64Read");
    static SimpleMutex _curTimeMicros64ResyncMutex("curTimeMicros64Resync");

    static unsigned long long resyncTime() {
        SimpleMutex::scoped_lock lkResync(_curTimeMicros64ResyncMutex);
        unsigned long long ftOld;
        unsigned long long ftNew;
        ftOld = ftNew = getFiletime();
        do {
            ftNew = getFiletime();
        } while (ftOld == ftNew);   // wait for filetime to change

        unsigned long long newPerfCounter = getPerfCounter();

        // Make sure that we use consistent values for baseFiletime and basePerfCounter.
        //
        SimpleMutex::scoped_lock lkRead(_curTimeMicros64ReadMutex);
        baseFiletime = ftNew;
        basePerfCounter = newPerfCounter;
        resyncInterval = 60 * Timer::_countsPerSecond;
        return newPerfCounter;
    }

    unsigned long long curTimeMicros64() {

        // Get a current value for QueryPerformanceCounter; if it is not time to resync we will
        // use this value.
        //
        unsigned long long perfCounter = getPerfCounter();

        // Periodically resync the timer so that we don't let timer drift accumulate.  Testing
        // suggests that we drift by about one microsecond per minute, so resynching once per
        // minute should keep drift to no more than one microsecond.
        //
        if ((perfCounter - basePerfCounter) > resyncInterval) {
            perfCounter = resyncTime();
        }

        // Make sure that we use consistent values for baseFiletime and basePerfCounter.
        //
        SimpleMutex::scoped_lock lkRead(_curTimeMicros64ReadMutex);

        // Compute the current time in FILETIME format by adding our base FILETIME and an offset
        // from that time based on QueryPerformanceCounter.  The math is (logically) to compute the
        // fraction of a second elapsed since 'baseFiletime' by taking the difference in ticks
        // and dividing by the tick frequency, then scaling this fraction up to units of 100
        // nanoseconds to match the FILETIME format.  We do the multiplication first to avoid
        // truncation while using only integer instructions.
        //
        unsigned long long computedTime = baseFiletime +
                ((perfCounter - basePerfCounter) * 10 * 1000 * 1000) / Timer::_countsPerSecond;

        // Convert the computed FILETIME into microseconds since the Unix epoch (1/1/1970).
        //
        return boost::date_time::winapi::file_time_to_microseconds(computedTime);
    }

    unsigned curTimeMicros() {
        boost::xtime xt;
        boost::xtime_get(&xt, MONGO_BOOST_TIME_UTC);
        unsigned t = xt.nsec / 1000;
        unsigned secs = xt.sec % 1024;
        return secs*1000000 + t;
    }
#else
#  include <sys/time.h>
    unsigned long long curTimeMillis64() {
        timeval tv;
        gettimeofday(&tv, NULL);
        return ((unsigned long long)tv.tv_sec) * 1000 + tv.tv_usec / 1000;
    }
    Date_t jsTime() {
        timeval tv;
        gettimeofday(&tv, NULL);
        unsigned long long t = tv.tv_usec / 1000;
        return ((unsigned long long) tv.tv_sec * 1000) + t + getJSTimeVirtualSkew() + getJSTimeVirtualThreadSkew();
    }
    unsigned long long curTimeMicros64() {
        timeval tv;
        gettimeofday(&tv, NULL);
        return (((unsigned long long) tv.tv_sec) * 1000*1000) + tv.tv_usec;
    }
    unsigned curTimeMicros() {
        timeval tv;
        gettimeofday(&tv, NULL);
        unsigned secs = tv.tv_sec % 1024;
        return secs*1000*1000 + tv.tv_usec;
    }
#endif

}  // namespace mongo
