// @file dur.h durability support

/**
*    Copyright (C) 2009 10gen Inc.
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

#pragma once

#include <boost/filesystem/operations.hpp>
#include <boost/shared_ptr.hpp>
#include <list>

#include "mongo/db/storage/mmap_v1/dur_journalformat.h"
#include "mongo/util/concurrency/mutex.h"

namespace mongo {

class DurableMappedFile;

namespace dur {

struct ParsedJournalEntry;

/** call go() to execute a recovery from existing journal files.
 */
class RecoveryJob {
    MONGO_DISALLOW_COPYING(RecoveryJob);

public:
    RecoveryJob();
    ~RecoveryJob();

    void go(std::vector<boost::filesystem::path>& files);

    /** @param data data between header and footer. compressed if recovering. */
    void processSection(const JSectHeader* h, const void* data, unsigned len, const JSectFooter* f);

    // locks and calls _close()
    void close();

    static RecoveryJob& get() {
        return _instance;
    }

private:
    class Last {
    public:
        Last();
        DurableMappedFile* newEntry(const ParsedJournalEntry&, RecoveryJob&);

    private:
        DurableMappedFile* mmf;
        std::string dbName;
        int fileNo;
    };


    void write(Last& last, const ParsedJournalEntry& entry);  // actually writes to the file
    void applyEntry(Last& last, const ParsedJournalEntry& entry, bool apply, bool dump);
    void applyEntries(const std::vector<ParsedJournalEntry>& entries);
    bool processFileBuffer(const void*, unsigned len);
    bool processFile(boost::filesystem::path journalfile);
    void _close();  // doesn't lock


    // Set of memory mapped files and a mutex to protect them
    mongo::mutex _mx;
    std::list<boost::shared_ptr<DurableMappedFile>> _mmfs;

    // Are we in recovery or WRITETODATAFILES
    bool _recovering;

    unsigned long long _lastDataSyncedFromLastRun;
    unsigned long long _lastSeqMentionedInConsoleLog;


    static RecoveryJob& _instance;
};


void replayJournalFilesAtStartup();
}
}
