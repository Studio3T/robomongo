// top.h : DB usage monitor.

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

#include <boost/date_time/posix_time/posix_time.hpp>

#include "mongo/util/concurrency/mutex.h"
#include "mongo/util/string_map.h"

namespace mongo {

/**
 * tracks usage by collection
 */
class Top {
public:
    Top() : _lock("Top") {}

    struct UsageData {
        UsageData() : time(0), count(0) {}
        UsageData(const UsageData& older, const UsageData& newer);
        long long time;
        long long count;

        void inc(long long micros) {
            count++;
            time += micros;
        }
    };

    struct CollectionData {
        /**
         * constructs a diff
         */
        CollectionData() {}
        CollectionData(const CollectionData& older, const CollectionData& newer);

        UsageData total;

        UsageData readLock;
        UsageData writeLock;

        UsageData queries;
        UsageData getmore;
        UsageData insert;
        UsageData update;
        UsageData remove;
        UsageData commands;
    };

    typedef StringMap<CollectionData> UsageMap;

public:
    void record(const StringData& ns, int op, int lockType, long long micros, bool command);
    void append(BSONObjBuilder& b);
    void cloneMap(UsageMap& out) const;
    void collectionDropped(const StringData& ns);

public:  // static stuff
    static Top global;

private:
    void _appendToUsageMap(BSONObjBuilder& b, const UsageMap& map) const;
    void _appendStatsEntry(BSONObjBuilder& b, const char* statsName, const UsageData& map) const;
    void _record(CollectionData& c, int op, int lockType, long long micros, bool command);

    mutable SimpleMutex _lock;
    UsageMap _usage;
    std::string _lastDropped;
};

}  // namespace mongo
