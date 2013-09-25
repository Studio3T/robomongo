#include "MongoCollectionInfo.h"
#include "robomongo/core/utils/BsonUtils.h"
#include <mongo/client/dbclient.h>

namespace Robomongo
{
    MongoCollectionInfo::MongoCollectionInfo(mongo::BSONObj stats) : _ns(stats.getStringField("ns"))
    {
        // if "size" and "storageSize" are of type Int32 or Int64, they
        // will be converted to double by "numberDouble()" function.
        _sizeBytes = BsonUtils::getField<mongo::NumberDouble>(stats,"size");
        _storageSizeBytes = BsonUtils::getField<mongo::NumberDouble>(stats,"storageSize");

        _count = BsonUtils::getField<mongo::NumberInt>(stats,"count");
    }
}

