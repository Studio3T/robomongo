#include "robomongo/core/domain/MongoQueryInfo.h"

#include <mongo/client/dbclient.h>

namespace Robomongo
{
    namespace detail
    {
        std::string prepareServerAddress(const std::string &address)
        {
            size_t pos = address.find_first_of("[");
            if (pos!=std::string::npos){
                return address.substr(0,pos);
            }       
            return address;
        }
    }

    MongoQueryInfo::MongoQueryInfo() {}

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
        _special(special)
        {}

    bool MongoQueryInfo::isValid() const
    {
        return !_serverAddress.empty() && !_databaseName.empty() && !_collectionName.empty();
    }
}
