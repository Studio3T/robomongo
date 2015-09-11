/**
 *    Copyright (C) 2014 MongoDB Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
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

#include "mongo/platform/basic.h"

#include "mongo/base/init.h"
#include "mongo/db/global_environment_experiment.h"
#include "mongo/db/storage_options.h"
#include "mongo/db/storage/kv/kv_storage_engine.h"
#include "mongo/db/storage/storage_engine_metadata.h"
#include "mongo/util/mongoutils/str.h"

#include "rocks_engine.h"
#include "rocks_server_status.h"
#include "rocks_parameters.h"

namespace mongo {
const std::string kRocksDBEngineName = "rocksdb";

namespace {

class RocksFactory : public StorageEngine::Factory {
public:
    virtual ~RocksFactory() {}
    virtual StorageEngine* create(const StorageGlobalParams& params,
                                  const StorageEngineLockFile& lockFile) const {
        KVStorageEngineOptions options;
        options.directoryPerDB = params.directoryperdb;
        options.forRepair = params.repair;
        // Mongo keeps some files in params.dbpath. To avoid collision, put out files under
        // db/ directory
        auto engine = new RocksEngine(params.dbpath + "/db", params.dur);
        // Intentionally leaked.
        auto leaked __attribute__((unused)) = new RocksServerStatusSection(engine);
        auto leaked2 __attribute__((unused)) = new RocksRateLimiterServerParameter(engine);
        auto leaked3 __attribute__((unused)) = new RocksBackupServerParameter(engine);
        auto leaked4 __attribute__((unused)) = new RocksCompactServerParameter(engine);
        auto leaked5 __attribute__((unused)) = new RocksCacheSizeParameter(engine);

        return new KVStorageEngine(engine, options);
    }

    virtual StringData getCanonicalName() const {
        return kRocksDBEngineName;
    }

    virtual Status validateCollectionStorageOptions(const BSONObj& options) const {
        return Status::OK();
    }

    virtual Status validateIndexStorageOptions(const BSONObj& options) const {
        return Status::OK();
    }

    virtual Status validateMetadata(const StorageEngineMetadata& metadata,
                                    const StorageGlobalParams& params) const {
        const BSONObj& options = metadata.getStorageEngineOptions();
        BSONElement element = options.getField(kRocksFormatVersionString);
        if (element.eoo() || !element.isNumber()) {
            return Status(ErrorCodes::UnsupportedFormat,
                          "Storage engine metadata format not recognized. If you created "
                          "this database with older version of mongo, please reload the "
                          "database using mongodump and mongorestore");
        }
        if (element.numberInt() != kRocksFormatVersion) {
            return Status(ErrorCodes::UnsupportedFormat,
                          str::stream()
                              << "Database created with format version " << element.numberInt()
                              << " and this version only supports format version "
                              << kRocksFormatVersion
                              << ". Please reload the database using mongodump and mongorestore");
        }
        return Status::OK();
    }

    virtual BSONObj createMetadataOptions(const StorageGlobalParams& params) const {
        BSONObjBuilder builder;
        builder.append(kRocksFormatVersionString, kRocksFormatVersion);
        return builder.obj();
    }

private:
    // Current disk format. We bump this number when we change the disk format. MongoDB will
    // fail to start if the versions don't match. In that case a user needs to run mongodump
    // and mongorestore.
    // * Version 1 was the format with many column families -- one column family for each
    // collection and index
    // * Version 2 keeps all collections and indexes in a single column family
    // * Version 3 (current) reserves two prefixes for oplog. one prefix keeps the oplog
    // documents and another only keeps keys. That way, we can cleanup the oplog without
    // reading full documents
    // oplog cleanup
    const int kRocksFormatVersion = 2;
    const std::string kRocksFormatVersionString = "rocksFormatVersion";
};
}  // namespace

MONGO_INITIALIZER_WITH_PREREQUISITES(RocksEngineInit, ("SetGlobalEnvironment"))
(InitializerContext* context) {
    getGlobalEnvironment()->registerStorageEngine(kRocksDBEngineName, new RocksFactory());
    return Status::OK();
}
}
