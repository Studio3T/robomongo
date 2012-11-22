#ifndef MONGODATABASE_H
#define MONGODATABASE_H

#include <QObject>
#include "Core.h"

namespace Robomongo
{
    class MongoDatabase : public QObject
    {
        Q_OBJECT

    public:

        /**
         * @brief MongoDatabase
         * @param server - pointer to parent MongoServer
         */
        MongoDatabase(const MongoServer *server, const QString &name);
        ~MongoDatabase();

        QString name() const { return _name; }

        /**
         * @brief Checks that this is a system database
         * @return true if system, false otherwise
         */
        bool isSystem() const { return _system; }


    private:

        const MongoServer *_server;
        QString _name;
        bool _system;

    };

}

#endif // MONGODATABASE_H
