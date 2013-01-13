#include "robomongo/core/domain/MongoCollection.h"

using namespace Robomongo;

MongoCollection::MongoCollection(MongoDatabase *database, const MongoCollectionInfo &info) :
    QObject(),
    _ns(info.ns()),
    _database(database),
    _info(info),
    _system(false)
{
    // system databases starts from system.*
    if (_ns.collectionName().startsWith("system."))
        _system = true;
}

QString MongoCollection::sizeString() const
{
    return buildNiceSizeString(_info.sizeBytes());
}

QString MongoCollection::storageSizeString() const
{
    return buildNiceSizeString(_info.storageSizeBytes());
}

QString MongoCollection::buildNiceSizeString(int size) const
{
    if (size < 1024 * 100) {
        double kb = ((double) size) / 1024;
        return QString("%1 kb").arg(kb, 2, 'f', 2);
    }

    double mb = ((double) size) / 1024 / 1024;
    return QString("%1 mb").arg(mb, 2, 'f', 2);
}
