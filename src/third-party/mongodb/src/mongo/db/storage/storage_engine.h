// storage_engine.h

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

#include <string>
#include <vector>

#include "mongo/base/status.h"
#include "mongo/bson/bsonobj.h"
#include "mongo/util/mongoutils/str.h"

namespace mongo {

class DatabaseCatalogEntry;
class OperationContext;
class RecoveryUnit;
struct StorageGlobalParams;
class StorageEngineLockFile;
class StorageEngineMetadata;

/**
 * The StorageEngine class is the top level interface for creating a new storage
 * engine.  All StorageEngine(s) must be registered by calling registerFactory in order
 * to possibly be activated.
 */
class StorageEngine {
public:
    /**
     * The interface for creating new instances of storage engines.
     *
     * A storage engine provides an instance of this class (along with an associated
     * name) to the global environment, which then sets the global storage engine
     * according to the provided configuration parameter.
     */
    class Factory {
    public:
        virtual ~Factory() {}

        /**
         * Return a new instance of the StorageEngine.  Caller owns the returned pointer.
         */
        virtual StorageEngine* create(const StorageGlobalParams& params,
                                      const StorageEngineLockFile& lockFile) const = 0;

        /**
         * Returns the name of the storage engine.
         *
         * Implementations that change the value of the returned string can cause
         * data file incompatibilities.
         */
        virtual StringData getCanonicalName() const = 0;

        /**
         * Validates creation options for a collection in the StorageEngine.
         * Returns an error if the creation options are not valid.
         *
         * Default implementation only accepts empty objects (no options).
         */
        virtual Status validateCollectionStorageOptions(const BSONObj& options) const {
            if (options.isEmpty())
                return Status::OK();
            return Status(ErrorCodes::InvalidOptions,
                          str::stream() << "storage engine " << getCanonicalName()
                                        << " does not support any collection storage options");
        }

        /**
         * Validates creation options for an index in the StorageEngine.
         * Returns an error if the creation options are not valid.
         *
         * Default implementation only accepts empty objects (no options).
         */
        virtual Status validateIndexStorageOptions(const BSONObj& options) const {
            if (options.isEmpty())
                return Status::OK();
            return Status(ErrorCodes::InvalidOptions,
                          str::stream() << "storage engine " << getCanonicalName()
                                        << " does not support any index storage options");
        }

        /**
         * Validates existing metadata in the data directory against startup options.
         * Returns an error if the storage engine initialization should not proceed
         * due to any inconsistencies between the current startup options and the creation
         * options stored in the metadata.
         */
        virtual Status validateMetadata(const StorageEngineMetadata& metadata,
                                        const StorageGlobalParams& params) const = 0;

        /**
         * Returns a new document suitable for storing in the data directory metadata.
         * This document will be used by validateMetadata() to check startup options
         * on restart.
         */
        virtual BSONObj createMetadataOptions(const StorageGlobalParams& params) const = 0;
    };

    /**
     * Called after the globalStorageEngine pointer has been set up, before any other methods
     * are called. Any initialization work that requires the ability to create OperationContexts
     * should be done here rather than in the constructor.
     */
    virtual void finishInit() {}

    /**
     * Returns a new interface to the storage engine's recovery unit.  The recovery
     * unit is the durability interface.  For details, see recovery_unit.h
     *
     * Caller owns the returned pointer.
     */
    virtual RecoveryUnit* newRecoveryUnit() = 0;

    /**
     * List the databases stored in this storage engine.
     *
     * XXX: why doesn't this take OpCtx?
     */
    virtual void listDatabases(std::vector<std::string>* out) const = 0;

    /**
     * Return the DatabaseCatalogEntry that describes the database indicated by 'db'.
     *
     * StorageEngine owns returned pointer.
     * It should not be deleted by any caller.
     */
    virtual DatabaseCatalogEntry* getDatabaseCatalogEntry(OperationContext* opCtx,
                                                          const StringData& db) = 0;

    /**
     * Returns whether the storage engine supports its own locking locking below the collection
     * level. If the engine returns true, MongoDB will acquire intent locks down to the
     * collection level and will assume that the engine will ensure consistency at the level of
     * documents. If false, MongoDB will lock the entire collection in Shared/Exclusive mode
     * for read/write operations respectively.
     */
    virtual bool supportsDocLocking() const = 0;

    /**
     * Returns if the engine supports a journalling concept.
     * This controls whether awaitCommit gets called or fsync to ensure data is on disk.
     */
    virtual bool isDurable() const = 0;

    /**
     * Only MMAPv1 should override this and return true to trigger MMAPv1-specific behavior.
     */
    virtual bool isMmapV1() const {
        return false;
    }

    /**
     * Closes all file handles associated with a database.
     */
    virtual Status closeDatabase(OperationContext* txn, const StringData& db) = 0;

    /**
     * Deletes all data and metadata for a database.
     */
    virtual Status dropDatabase(OperationContext* txn, const StringData& db) = 0;

    /**
     * @return number of files flushed
     */
    virtual int flushAllFiles(bool sync) = 0;

    /**
     * Recover as much data as possible from a potentially corrupt RecordStore.
     * This only recovers the record data, not indexes or anything else.
     *
     * Generally, this method should not be called directly except by the repairDatabase()
     * free function.
     *
     * NOTE: MMAPv1 does not support this method and has its own repairDatabase() method.
     */
    virtual Status repairRecordStore(OperationContext* txn, const std::string& ns) = 0;

    /**
     * This method will be called before there is a clean shutdown.  Storage engines should
     * override this method if they have clean-up to do that is different from unclean shutdown.
     * MongoDB will not call into the storage subsystem after calling this function.
     *
     * There is intentionally no uncleanShutdown().
     */
    virtual void cleanShutdown() = 0;

protected:
    /**
     * The destructor will never be called. See cleanShutdown instead.
     */
    virtual ~StorageEngine() {}
};

}  // namespace mongo
