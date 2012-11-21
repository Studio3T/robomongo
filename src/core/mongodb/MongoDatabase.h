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


    private:

        const MongoServer *_server;
        QString _name;

    };

}

#endif // MONGODATABASE_H
