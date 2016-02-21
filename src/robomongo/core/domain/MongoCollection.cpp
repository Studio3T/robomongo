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
        // System databases starts from system.*
        std::string collectionName = _ns.collectionName();
        std::string prefix = "system.";

        // Checking whether `collectionName` starts from `system`
        if (collectionName.compare(0, prefix.length(), prefix) == 0)
            _system = true;
    }

/*    std::string MongoCollection::sizeString() const
    {
        return MongoUtils::buildNiceSizeString(_info.sizeBytes()).toStdString();
    }

    QString MongoCollection::storageSizeString() const
    {
        return MongoUtils::buildNiceSizeString(_info.storageSizeBytes());
    }*/
}
