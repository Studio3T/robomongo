#pragma once

#include <mongo/client/dbclientinterface.h>
#include <mongo/bson/bsonobj.h>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoQueryInfo.h"
#include "robomongo/core/domain/MongoUser.h"
#include "robomongo/core/domain/MongoFunction.h"
#include "robomongo/core/events/MongoEventsInfo.h"

namespace Robomongo
{
    class MongoClient
    {
    public:
        MongoClient(mongo::DBClientBase *const scopedConnection);

        std::vector<std::string> getCollectionNamesWithDbname(const std::string &dbname) const;
        std::vector<std::string> getDatabaseNames() const;
        float getVersion() const;
        std::string getStorageEngineType() const;

        std::vector<MongoUser> getUsers(const std::string &dbName);
        void createUser(const std::string &dbName, const MongoUser &user, bool overwrite);
        void dropUser(const std::string &dbName, const mongo::OID &id);

        std::vector<MongoFunction> getFunctions(const std::string &dbName);
        std::vector<EnsureIndexInfo> getIndexes(const MongoCollectionInfo &collection) const;
        void dropIndexFromCollection(const MongoCollectionInfo &collection, const std::string &indexName) const;
        void ensureIndex(const EnsureIndexInfo &oldInfo, const EnsureIndexInfo &newInfo) const;

        void renameIndexFromCollection(const MongoCollectionInfo &collection, const std::string &oldIndexName,
                                       const std::string &newIndexName) const;

        void createFunction(const std::string &dbName, const MongoFunction &fun,
                            const std::string &existingFunctionName = std::string());

        void dropFunction(const std::string &dbName, const std::string &name);
        void createDatabase(const std::string &dbName);
        void dropDatabase(const std::string &dbName);

        void createCollection(const std::string &ns, long long size, bool capped, int max, const mongo::BSONObj& extraOptions, mongo::BSONObj* info = nullptr);
        void renameCollection(const MongoNamespace &ns, const std::string &newCollectionName);
        void duplicateCollection(const MongoNamespace &ns, const std::string &newCollectionName);
        void dropCollection(const MongoNamespace &ns);
        void copyCollectionToDiffServer(mongo::DBClientBase *const, const MongoNamespace &from, const MongoNamespace &to);

        void insertDocument(const mongo::BSONObj &obj, const MongoNamespace &ns);
        void saveDocument(const mongo::BSONObj &obj, const MongoNamespace &ns);
        void removeDocuments(const MongoNamespace &ns, mongo::Query query, bool justOne = true);
        std::vector<MongoDocumentPtr> query(const MongoQueryInfo &info);

        MongoCollectionInfo runCollStatsCommand(const std::string &ns);
        std::vector<MongoCollectionInfo> runCollStatsCommand(const std::vector<std::string> &namespaces);

        void done();

    private:
        mongo::DBClientBase *const _dbclient;
        void checkLastErrorAndThrow(const std::string &db);
    };
}
