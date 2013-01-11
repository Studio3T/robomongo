#include "robomongo/core/domain/MongoCollection.h"

using namespace Robomongo;

MongoCollection::MongoCollection(MongoDatabase *database, const CollectionInfo &info) :
    QObject(),
    _database(database),
    _info(info),
    _fullName(info.collectionName),
    _system(false)
{
    // full name contains db name, so we are extracting name of collection:
    int dot = _fullName.indexOf('.');
    _name = _fullName.mid(dot + 1);

    // system databases starts from system.*
    if (_name.startsWith("system."))
        _system = true;
}

QString MongoCollection::sizeString() const
{
    return buildNiceSizeString(_info.sizeBytes);
}

QString MongoCollection::storageSizeString() const
{
    return buildNiceSizeString(_info.storageSizeBytes);
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
