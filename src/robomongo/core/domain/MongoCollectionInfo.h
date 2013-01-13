#pragma once

#include <QString>
#include <mongo/client/dbclient.h>

#include "robomongo/core/domain/MongoNamespace.h"

namespace Robomongo
{
    class MongoCollectionInfo
    {
    public:
        MongoCollectionInfo(mongo::BSONObj stats);

        QString name() const { return _ns.collectionName(); }
        QString fullName() const { return _ns.toString(); }
        MongoNamespace ns() const { return _ns; }

        int sizeBytes() const { return _sizeBytes; }
        int storageSizeBytes() const { return _storageSizeBytes; }
        int count() const { return _count; }

    private:
        MongoNamespace _ns;
        int _sizeBytes;
        int _storageSizeBytes;
        int _count;
    };
}

