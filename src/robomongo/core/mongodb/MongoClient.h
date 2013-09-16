#pragma once

#include <QStringList>
#include "mongo/client/dbclientinterface.h"

#include "robomongo/core/domain/MongoQueryInfo.h"
#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/domain/MongoUser.h"
#include "robomongo/core/domain/MongoFunction.h"
#include "robomongo/core/events/MongoEventsInfo.h"

namespace Robomongo
{
    class MongoClient
    {
    public:
        MongoClient(mongo::DBClientBase *const scopedConnection);

        std::vector<std::string> getCollectionNames(const std::string &dbname) const;
        std::vector<std::string> getDatabaseNames() const;

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

        void createCollection(const std::string &dbName, const std::string &collectionName);
        void renameCollection(const std::string &dbName, const std::string &collectionName, const std::string &newCollectionName);
        void duplicateCollection(const std::string &dbName, const std::string &collectionName, const std::string &newCollectionName);
        void dropCollection(const std::string &dbName, const std::string &collectionName);

        void insertDocument(const mongo::BSONObj &obj, const std::string &db, const std::string &collection);
        void saveDocument(const mongo::BSONObj &obj, const std::string &db, const std::string &collection);
        void removeDocuments(const std::string &db, const std::string &collection, mongo::Query query, bool justOne = true);
        std::vector<MongoDocumentPtr> query(const MongoQueryInfo &info);

        MongoCollectionInfo runCollStatsCommand(const std::string &ns);
        std::vector<MongoCollectionInfo> runCollStatsCommand(const std::vector<std::string> &namespaces);

        void done();

    private:
        mongo::DBClientBase *const _dbclient;
    };
}
