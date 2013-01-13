#include "MongoUtils.h"

using namespace Robomongo;

QString MongoUtils::getName(const QString &fullName)
{
    int dot = fullName.indexOf('.');
    return fullName.mid(dot + 1);
}
