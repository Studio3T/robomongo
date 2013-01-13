#include "MongoNamespace.h"

using namespace Robomongo;

MongoNamespace::MongoNamespace(const QString &ns) :
    _ns(ns)
{
    int dot = ns.indexOf('.');
    _collectionName = ns.mid(dot + 1);
    _databaseName = ns.mid(0, dot);
}

MongoNamespace::MongoNamespace(const QString &database, const QString &collection) :
    _databaseName(database),
    _collectionName(collection)
{
    _ns = QString("%1.%2").arg(_databaseName, _collectionName);
}
