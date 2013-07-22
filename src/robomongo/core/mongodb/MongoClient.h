#pragma once

#include <QStringList>
#include "mongo/client/dbclientinterface.h"

#include "robomongo/core/domain/MongoCollectionInfo.h"
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

        QStringList getCollectionNames(const QString &dbname) const;
        QStringList getDatabaseNames()const;

        QList<MongoUser> getUsers(const QString &dbName);
        void createUser(const QString &dbName, const MongoUser &user, bool overwrite);
        void dropUser(const QString &dbName, const mongo::OID &id);

        QList<MongoFunction> getFunctions(const QString &dbName);
        QList<EnsureIndexInfo> getIndexes(const MongoCollectionInfo &collection)const;
        void dropIndexFromCollection(const MongoCollectionInfo &collection,const QString &indexName)const;
        void ensureIndex(const EnsureIndexInfo &oldInfo,const EnsureIndexInfo &newInfo)const;

        void renameIndexFromCollection(const MongoCollectionInfo &collection, const QString &oldIndexName, const QString &newIndexName)const;

        void createFunction(const QString &dbName, const MongoFunction &fun, const QString &existingFunctionName = QString());
        void dropFunction(const QString &dbName, const QString &name);

        void createDatabase(const QString &dbName);
        void dropDatabase(const QString &dbName);

        void createCollection(const QString &dbName, const QString &collectionName);
        void renameCollection(const QString &dbName, const QString &collectionName, const QString &newCollectionName);
        void duplicateCollection(const QString &dbName, const QString &collectionName, const QString &newCollectionName);
        void dropCollection(const QString &dbName, const QString &collectionName);

        void insertDocument(const mongo::BSONObj &obj, const QString &db, const QString &collection);
        void saveDocument(const mongo::BSONObj &obj, const QString &db, const QString &collection);
        void removeDocuments(const QString &db, const QString &collection, mongo::Query query, bool justOne = true);
        QList<MongoDocumentPtr> query(const MongoQueryInfo &info);

        MongoCollectionInfo runCollStatsCommand(const QString &ns);
        QList<MongoCollectionInfo> runCollStatsCommand(const QStringList &namespaces);

        void done();

    private:
        mongo::DBClientBase *const _dbclient;
    };
}
