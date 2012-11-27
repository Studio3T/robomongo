#include "MongoCollection.h"

using namespace Robomongo;

MongoCollection::MongoCollection(MongoDatabase *database, const QString &fullName) :
    QObject(),
    _database(database),
    _fullName(fullName),
    _system(false)
{
    // full name contains db name, so we are extracting name of collection:
    int dot = _fullName.indexOf('.');
    _name = _fullName.mid(dot + 1);

    // system databases starts from system.*
    if (_name.startsWith("system."))
        _system = true;
}
