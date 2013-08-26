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
        std::string collectionName = _ns.collectionName();
        size_t pos = collectionName.find_first_of("system.");
        if (pos!=std::string::npos)
            _system = true;
    }

    std::string MongoCollection::sizeString() const
    {
        return MongoUtils::buildNiceSizeString(_info.sizeBytes()).toStdString();
    }

    QString MongoCollection::storageSizeString() const
    {
        return MongoUtils::buildNiceSizeString(_info.storageSizeBytes());
    }
}
