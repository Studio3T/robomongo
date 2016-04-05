#pragma once
#include <mongo/bson/bsonobj.h>
#include "robomongo/core/domain/MongoNamespace.h"

namespace Robomongo
{
    class MongoCollectionInfo
    {
    public:
        MongoCollectionInfo() {}
        MongoCollectionInfo(const std::string &ns);
//        MongoCollectionInfo(mongo::BSONObj stats);

        std::string name() const { return _ns.collectionName(); }
        std::string fullName() const { return _ns.toString(); }
        MongoNamespace ns() const { return _ns; }

        /**
         * @brief Size in bytes
         * It is double, because db.stats()'s "size" field may be double
         * for large values, while Int32 for small.
         */
//        double sizeBytes() const { return _sizeBytes; }

        /**
         * @brief Storage size in bytes
         * It is double, because db.stats()'s "storageSize" field may be double
         * for large values, while Int32 for small.
         */
//        double storageSizeBytes() const { return _storageSizeBytes; }

//        long long count() const { return _count; }

    private:
        MongoNamespace _ns;

        /**
         * @brief Size in bytes
         * It is double, because db.stats()'s "size" field may be double
         * for large values, while Int32 for small.
         */
        double _sizeBytes;

        /**
         * @brief Storage size in bytes
         * It is double, because db.stats()'s "storageSize" field may be double
         * for large values, while Int32 for small.
         */
        double _storageSizeBytes;

        long long _count;
    };
}

