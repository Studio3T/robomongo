#pragma once

#include <QString>
#include <mongo/bson/bsonobj.h>

namespace Robomongo
{
    struct MongoQueryInfo
    {
        MongoQueryInfo();

        MongoQueryInfo(const std::string &server, const std::string &database, const std::string &collection,
                  mongo::BSONObj query, mongo::BSONObj fields, int limit, int skip, int batchSize,
                  int options, bool special);

        std::string _serverAddress;
        std::string _databaseName;
        std::string _collectionName;
        mongo::BSONObj _query;
        mongo::BSONObj _fields;
        int _limit;
        int _skip;
        int _batchSize;
        int _options;
        bool _special; // flag, indicating that `query` contains special fields on
                      // first level, and query data in `query` field.
        bool _isNull;
    };
}
