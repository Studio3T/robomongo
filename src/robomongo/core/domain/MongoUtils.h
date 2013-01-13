#pragma once

#include <QString>

namespace Robomongo
{
    class MongoUtils
    {
    public:
        MongoUtils();

        /**
         * @brief Extracts short name from fullName (i.e. collection namespace)
         */
        static QString getName(const QString &fullName);
    };
}
