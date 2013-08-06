#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/domain/MongoUtils.h"

namespace Robomongo
{
    MongoCollection::MongoCollection(MongoDatabase *database, const MongoCollectionInfo &info) :
        _ns(info.ns()),
        _database(database),
        _info(info),
        _system(false)
    {
        // system databases starts from system.*
        if (_ns.collectionName().startsWith("system."))
            _system = true;
    }

    QString MongoCollection::sizeString() const
    {
        return MongoUtils::buildNiceSizeString(_info.sizeBytes());
    }

    QString MongoCollection::storageSizeString() const
    {
        return MongoUtils::buildNiceSizeString(_info.storageSizeBytes());
    }
}
