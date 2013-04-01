#include "MongoCollectionInfo.h"

using namespace Robomongo;

MongoCollectionInfo::MongoCollectionInfo(mongo::BSONObj stats)
{
    _ns = MongoNamespace(QString::fromStdString(stats.getStringField("ns")));

    // if "size" and "storageSize" are of type Int32 or Int64, they
    // will be converted to double by "numberDouble()" function.
    _sizeBytes = stats.getField("size").numberDouble();
    _storageSizeBytes = stats.getField("storageSize").numberDouble();

    _count = stats.getIntField("count");
}

