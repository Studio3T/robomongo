#include "robomongo/core/domain/MongoQueryInfo.h"

#include <mongo/client/dbclient.h>

namespace Robomongo
{
    MongoQueryInfo::MongoQueryInfo() :
        _isNull(true) {}

    MongoQueryInfo::MongoQueryInfo(const std::string &server, const std::string &database, const std::string &collection,
              mongo::BSONObj query, mongo::BSONObj fields, int limit, int skip, int batchSize,
              int options, bool special) :
        _serverAddress(server),
        _databaseName(database),
        _collectionName(collection),
        _query(query),
        _fields(fields),
        _limit(limit),
        _skip(skip),
        _batchSize(batchSize),
        _options(options),
        _special(special),
        _isNull(false) {}
}
