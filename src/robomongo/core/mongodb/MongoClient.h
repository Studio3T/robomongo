#pragma once

#include <mongo/client/dbclient.h>
#include <boost/scoped_ptr.hpp>
#include <QStringList>

#include "robomongo/core/domain/MongoCollectionInfo.h"
#include "robomongo/core/domain/MongoQueryInfo.h"
#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/domain/MongoUser.h"
#include "robomongo/core/domain/MongoFunction.h"

namespace Robomongo
{
    class MongoClient
    {
    public:
        MongoClient(mongo::DBClientBase *scopedConnection);
        ~MongoClient() {}

        QStringList getCollectionNames(const QString &dbname);
        QStringList getDatabaseNames();

        QList<MongoUser> getUsers(const QString &dbName);
        void createUser(const QString &dbName, const MongoUser &user, bool overwrite);
        void dropUser(const QString &dbName, const mongo::OID &id);

        QList<MongoFunction> getFunctions(const QString &dbName);
        void createFunction(const QString &dbName, const MongoFunction &fun, const QString &existingFunctionName = QString());
        void dropFunction(const QString &dbName, const QString &name);

        void createDatabase(const QString &dbName);
        void dropDatabase(const QString &dbName);

        void createCollection(const QString &dbName, const QString &collectionName);
        void renameCollection(const QString &dbName, const QString &collectionName, const QString &newCollectionName);
        void dropCollection(const QString &dbName, const QString &collectionName);

        void insertDocument(const mongo::BSONObj &obj, const QString &db, const QString &collection);
        void saveDocument(const mongo::BSONObj &obj, const QString &db, const QString &collection);
        void removeDocuments(const QString &db, const QString &collection, mongo::Query query, bool justOne = true);
        QList<MongoDocumentPtr> query(const MongoQueryInfo &info);

        MongoCollectionInfo runCollStatsCommand(const QString &ns);
        QList<MongoCollectionInfo> runCollStatsCommand(const QStringList &namespaces);

        void done();

    private:
       mongo::DBClientBase *_dbclient;
    };
}
