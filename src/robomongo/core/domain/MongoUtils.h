#pragma once
#include <QString>
#include <mongo/bson/bsonobj.h>

namespace Robomongo
{
    namespace MongoUtils
    {
        /**
         * @brief Extracts short name from fullName (i.e. collection namespace)
         */
        QString getName(const QString &fullName);

        /**
         * @throws mongo::MsgAssertionException
         */
        mongo::BSONObj fromjson(const QString &text);

        QString buildNiceSizeString(double sizeBytes);
        QString buildPasswordHash(const QString &username, const QString &password);
    }
}
