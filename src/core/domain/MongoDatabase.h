#ifndef MONGODATABASE_H
#define MONGODATABASE_H

#include <QObject>
#include "mongo/client/dbclient.h"
#include "boost/shared_ptr.hpp"

#include "Core.h"
#include "events/MongoEvents.h"
#include "mongodb/MongoClient.h"
#include "MongoServer.h"

namespace Robomongo
{
    class Dispatcher;

    class MongoDatabase : public QObject, public boost::enable_shared_from_this<MongoDatabase>
    {
        Q_OBJECT

    public:

        /**
         * @brief MongoDatabase
         * @param server: pointer to parent MongoServer
         */
        MongoDatabase(MongoServer *server, const QString &name);
        ~MongoDatabase();

        /**
         * @brief Initiate listCollection asynchronous operation.
         *
         */
        void listCollections();

        QString name() const { return _name; }

        /**
         * @brief Checks that this is a system database
         * @return true if system, false otherwise
         */
        bool isSystem() const { return _system; }

        MongoServer *server() const { return _server; }

    protected slots:

        void handle(LoadCollectionNamesResponse *collectionNames);

    private:

        MongoServer *_server;
        MongoClient *_client;
        QString _name;
        bool _system;

        Dispatcher &_dispatcher;

    };

    class MongoDatabase_CollectionListLoadedEvent : public Event
    {
        R_EVENT

        MongoDatabase_CollectionListLoadedEvent(QObject *sender, const QList<MongoCollectionPtr> &list) :
            Event(sender),
            list(list) { }

        QList<MongoCollectionPtr> list;
    };
}

#endif // MONGODATABASE_H
