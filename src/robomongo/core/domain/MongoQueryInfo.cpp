#include "MongoQueryInfo.h"
namespace Robomongo
{
    MongoQueryInfo::MongoQueryInfo() :
        isNull(true) {}

    MongoQueryInfo::MongoQueryInfo(const QString &server, const QString &database, const QString &collection,
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
}
