// timer_stats.cpp

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

#include "mongo/db/stats/timer_stats.h"

namespace mongo {

TimerHolder::TimerHolder(TimerStats* stats) : _stats(stats), _recorded(false) {}

TimerHolder::~TimerHolder() {
    if (!_recorded) {
        recordMillis();
    }
}

int TimerHolder::recordMillis() {
    _recorded = true;
    if (_stats) {
        return _stats->record(_t);
    }
    return _t.millis();
}

void TimerStats::recordMillis(int millis) {
    scoped_spinlock lk(_lock);
    _num++;
    _totalMillis += millis;
}

int TimerStats::record(const Timer& timer) {
    int millis = timer.millis();
    recordMillis(millis);
    return millis;
}

BSONObj TimerStats::getReport() const {
    long long n, t;
    {
        scoped_spinlock lk(_lock);
        n = _num;
        t = _totalMillis;
    }
    BSONObjBuilder b(64);
    b.appendNumber("num", n);
    b.appendNumber("totalMillis", t);
    return b.obj();
}
}
