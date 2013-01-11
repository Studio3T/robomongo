#pragma once

#include <QObject>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoDatabase.h"

namespace Robomongo
{
    class MongoCollection : public QObject
    {
        Q_OBJECT
    public:
        MongoCollection(MongoDatabase *database, const CollectionInfo &info);

        bool isSystem() const { return _system; }

        QString name() const { return _name; }
        const CollectionInfo &info() const { return _info; }
        QString fullName() const { return _fullName; }
        MongoDatabase *database() const { return _database; }

        QString sizeNice() const;

    private:
        /**
         * @brief Database that contains this collection
         */
        MongoDatabase *_database;

        /*
        ** Name of collection (without database prefix)
        */
        QString _name;

        /*
        ** Full name of collection (with database prefix)
        */
        QString _fullName;

        bool _system;

        CollectionInfo _info;
    };
}
