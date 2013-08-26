#include "robomongo/core/domain/MongoQueryInfo.h"
#include <mongo/client/dbclient.h>

namespace Robomongo
{
    MongoQueryInfo::MongoQueryInfo() :
        isNull(true) {}

    MongoQueryInfo::MongoQueryInfo(const std::string &server, const std::string &database, const std::string &collection,
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
