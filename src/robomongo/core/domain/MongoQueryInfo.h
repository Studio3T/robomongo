#pragma once
#include <QString>
#include <mongo/bson/bsonobj.h>

namespace Robomongo
{
    struct MongoQueryInfo
    {
        MongoQueryInfo();

        MongoQueryInfo(const QString &server, const QString &database, const QString &collection,
                  mongo::BSONObj query, mongo::BSONObj fields, int limit, int skip, int batchSize,
                  int options, bool special);

        QString serverAddress;
        QString databaseName;
        QString collectionName;
        mongo::BSONObj query;
        mongo::BSONObj fields;
        int limit;
        int skip;
        int batchSize;
        int options;
        bool special; // flag, indicating that `query` contains special fields on
                      // first level, and query data in `query` field.
        bool isNull;
    };
}
