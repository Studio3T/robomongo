#pragma once

#include <QString>
#include <mongo/client/dbclient.h>

namespace Robomongo
{
    class MongoQueryInfo
    {
    public:
        MongoQueryInfo() :
            isNull(true) {}

        MongoQueryInfo(const QString &server, const QString &database, const QString &collection,
                  mongo::BSONObj query, mongo::BSONObj fields, int limit, int skip, int batchSize,
                  int options, bool special) :
            serverAddress(server),
            databaseName(database),
            collectionName(collection),
            query(query),
            fields(fields),
            limit(limit),
            skip(skip),
            batchSize(batchSize),
            options(options),
            special(special),
            isNull(false) {}

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
