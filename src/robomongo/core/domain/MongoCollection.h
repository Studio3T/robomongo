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

        QString name() const { return _ns.collectionName(); }
        const MongoCollectionInfo info() const { return _info; }
        QString fullName() const { return _ns.toString(); }
        MongoDatabase *database() const { return _database; }

        QString sizeString() const;
        QString storageSizeString() const;

    private:

        /**
         * @brief Database that contains this collection
         */
        MongoDatabase *_database;
        bool _system;
        MongoCollectionInfo _info;
        MongoNamespace _ns;
    };
}
