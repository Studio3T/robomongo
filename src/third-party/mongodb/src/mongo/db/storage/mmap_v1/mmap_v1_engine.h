// mmap_v1_engine.h

/**
*    Copyright (C) 2014 MongoDB Inc.
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

#include <map>

#include <boost/thread/mutex.hpp>

#include "mongo/db/storage/storage_engine.h"
#include "mongo/db/storage/mmap_v1/record_access_tracker.h"

namespace mongo {

class MMAPV1DatabaseCatalogEntry;

class MMAPV1Engine : public StorageEngine {
public:
    MMAPV1Engine(const StorageEngineLockFile& lockFile);
    virtual ~MMAPV1Engine();

    void finishInit();

    RecoveryUnit* newRecoveryUnit();
    void listDatabases(std::vector<std::string>* out) const;
    int flushAllFiles(bool sync);

    DatabaseCatalogEntry* getDatabaseCatalogEntry(OperationContext* opCtx, const StringData& db);

    virtual bool supportsDocLocking() const {
        return false;
    }
    virtual bool isMmapV1() const {
        return true;
    }

    virtual bool isDurable() const;

    virtual Status closeDatabase(OperationContext* txn, const StringData& db);

    virtual Status dropDatabase(OperationContext* txn, const StringData& db);

    virtual void cleanShutdown();

    // Callers should use  repairDatabase instead.
    virtual Status repairRecordStore(OperationContext* txn, const std::string& ns) {
        return Status(ErrorCodes::InternalError, "MMAPv1 doesn't support repairRecordStore");
    }

    // MMAPv1 specific (non-virtual)
    Status repairDatabase(OperationContext* txn,
                          const std::string& dbName,
                          bool preserveClonedFilesOnFailure,
                          bool backupOriginalFiles);

    /**
     * Gets a reference to the abstraction used by MMAP v1 to track recently used memory
     * addresses.
     *
     * MMAPv1 specific (non-virtual). This is non-const because callers are allowed to use
     * the returned reference to modify the RecordAccessTracker.
     *
     * The RecordAccessTracker is thread-safe (it uses its own mutex internally).
     */
    RecordAccessTracker& getRecordAccessTracker();

private:
    static void _listDatabases(const std::string& directory, std::vector<std::string>* out);

    boost::mutex _entryMapMutex;
    typedef std::map<std::string, MMAPV1DatabaseCatalogEntry*> EntryMap;
    EntryMap _entryMap;

    // A record access tracker is essentially a large table which tracks recently used
    // addresses. It is used when higher layers (e.g. the query system) need to ask
    // the storage engine whether data is likely in physical memory.
    RecordAccessTracker _recordAccessTracker;
};

void _deleteDataFiles(const std::string& database);
}
