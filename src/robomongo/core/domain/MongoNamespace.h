#pragma once
#include <QString>

namespace Robomongo
{
    class MongoNamespace
    {
    public:
        MongoNamespace(const QString &ns);
        MongoNamespace(const QString &database, const QString &collection);
        MongoNamespace() {}
        QString toString() const { return _ns; }
        QString databaseName() const { return _databaseName; }
        QString collectionName() const { return _collectionName; }

    private:
        QString _ns;
        QString _databaseName;
        QString _collectionName;
    };
}
