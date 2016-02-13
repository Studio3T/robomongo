#include "MongoCollectionInfo.h"
#include "robomongo/core/utils/BsonUtils.h"
#include <mongo/client/dbclientinterface.h>

namespace Robomongo
{
    MongoCollectionInfo::MongoCollectionInfo(const std::string &ns) : _ns(ns) {}

/*    MongoCollectionInfo::MongoCollectionInfo(mongo::BSONObj stats) : _ns(stats.getStringField("ns"))
    {
        // if "size" and "storageSize" are of type Int32 or Int64, they
        // will be converted to double by "numberDouble()" function.
        _sizeBytes = BsonUtils::getField<mongo::NumberDouble>(stats,"size");
        _storageSizeBytes = BsonUtils::getField<mongo::NumberDouble>(stats,"storageSize");

        // NumberLong because of mongodb can have very big collections
        _count = BsonUtils::getField<mongo::NumberLong>(stats,"count");
    }*/
}

