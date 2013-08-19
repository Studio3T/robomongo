#pragma once
#include <QString>

namespace Robomongo
{
    namespace MongoUtils
    {
        QString buildNiceSizeString(double sizeBytes);
        QString buildPasswordHash(const QString &username, const QString &password);
    }
}
