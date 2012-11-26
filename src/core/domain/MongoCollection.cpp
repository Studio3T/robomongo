#include "MongoCollection.h"

using namespace Robomongo;

MongoCollection::MongoCollection(MongoDatabase *database, const QString &name) :
    QObject(),
    _database(database),
    _name(name),
    _system(false)
{
    // System databases starts from system.*
    if (name.startsWith("system."))
        _system = true;
}
