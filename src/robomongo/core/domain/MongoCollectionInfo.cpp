#include "MongoCollectionInfo.h"

using namespace Robomongo;

MongoCollectionInfo::MongoCollectionInfo(mongo::BSONObj stats)
{
    _ns = MongoNamespace(QString::fromStdString(stats.getStringField("ns")));
    _sizeBytes = stats.getIntField("size");
    _count = stats.getIntField("count");
    _storageSizeBytes = stats.getIntField("storageSize");
}

