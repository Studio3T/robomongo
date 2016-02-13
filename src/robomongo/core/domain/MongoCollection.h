#pragma once

#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/domain/MongoCollectionInfo.h"

namespace Robomongo
{
    class MongoCollection
    {
    public:
        MongoCollection(MongoDatabase *database, const MongoCollectionInfo &info);

        bool isSystem() const { return _system; }

        std::string name() const { return _ns.collectionName(); }
        const MongoCollectionInfo info() const { return _info; }
        std::string fullName() const { return _ns.toString(); }
        MongoDatabase *database() const { return _database; }

//        std::string sizeString() const;
//        QString storageSizeString() const;

    private:

        MongoDatabase *_database;
        bool _system;
        MongoCollectionInfo _info;
        MongoNamespace _ns;
    };
}
