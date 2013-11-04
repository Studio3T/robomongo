#pragma once

#include <mongo/client/dbclientinterface.h>
#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoQueryInfo.h"
#include "robomongo/core/domain/MongoUser.h"
#include "robomongo/core/domain/MongoFunction.h"
#include "robomongo/core/events/MongoEventsInfo.h"

namespace Robomongo
{
    namespace MongoClient
    {
        std::vector<std::string> getCollectionNames(mongo::DBClientBase *connection, const std::string &dbname);
        std::vector<std::string> getDatabaseNames(mongo::DBClientBase *connection);
        float getVersion(mongo::DBClientBase *connection);

        std::vector<MongoUser> getUsers(mongo::DBClientBase *connection, const std::string &dbName);
        void createUser(mongo::DBClientBase *connection, const std::string &dbName, const MongoUser &user, bool overwrite);
        void dropUser(mongo::DBClientBase *connection, const std::string &dbName, const mongo::OID &id);

        std::vector<MongoFunction> getFunctions(mongo::DBClientBase *connection, const std::string &dbName);
        std::vector<EnsureIndexInfo> getIndexes(mongo::DBClientBase *connection, const MongoCollectionInfo &collection);
        void dropIndexFromCollection(mongo::DBClientBase *connection, const MongoCollectionInfo &collection, const std::string &indexName);
        void ensureIndex(mongo::DBClientBase *connection, const EnsureIndexInfo &oldInfo, const EnsureIndexInfo &newInfo);

        void renameIndexFromCollection(mongo::DBClientBase *connection, const MongoCollectionInfo &collection, const std::string &oldIndexName,
                                       const std::string &newIndexName);

        void createFunction(mongo::DBClientBase *connection, const std::string &dbName, const MongoFunction &fun,
                            const std::string &existingFunctionName = std::string());

        void dropFunction(mongo::DBClientBase *connection, const std::string &dbName, const std::string &name);
        void createDatabase(mongo::DBClientBase *connection, const std::string &dbName);
        void dropDatabase(mongo::DBClientBase *connection, const std::string &dbName);

        void createCollection(mongo::DBClientBase *connection, const MongoNamespace &ns);
        void renameCollection(mongo::DBClientBase *connection, const MongoNamespace &ns, const std::string &newCollectionName);
        void duplicateCollection(mongo::DBClientBase *connection, const MongoNamespace &ns, const std::string &newCollectionName);
        void dropCollection(mongo::DBClientBase *connection, const MongoNamespace &ns);
        void copyCollectionToDiffServer(mongo::DBClientBase *connectionFrom, mongo::DBClientBase *connectionTo,const MongoNamespace &from, const MongoNamespace &to);

        bool insertDocument(mongo::DBClientBase *connection, const mongo::BSONObj &obj, const MongoNamespace &ns);
        bool saveDocument(mongo::DBClientBase *connection, const mongo::BSONObj &obj, const MongoNamespace &ns);
        void removeDocuments(mongo::DBClientBase *connection, const MongoNamespace &ns, mongo::Query query, bool justOne = true);
        std::vector<MongoDocumentPtr> query(mongo::DBClientBase *connection, const MongoQueryInfo &info);

        MongoCollectionInfo runCollStatsCommand(mongo::DBClientBase *connection, const std::string &ns);
        std::vector<MongoCollectionInfo> runCollStatsCommand(mongo::DBClientBase *connection, const std::vector<std::string> &namespaces);
    }
}
