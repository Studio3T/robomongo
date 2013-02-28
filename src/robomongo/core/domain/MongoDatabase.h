#pragma once

#include <QObject>
#include <mongo/client/dbclient.h>
#include <boost/shared_ptr.hpp>

#include "robomongo/core/Core.h"
#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/mongodb/MongoWorker.h"

namespace Robomongo
{
    class EventBus;

    /**
     * @brief Represents MongoDB database.
     */
    class MongoDatabase : public QObject
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
         */
        void loadCollections();

        void createCollection(const QString &collection);
        void dropCollection(const QString &collection);
        void renameCollection(const QString &collection, const QString &newCollection);

        QString name() const { return _name; }

        /**
         * @brief Checks that this is a system database.
         * @return true if system, false otherwise.
         */
        bool isSystem() const { return _system; }

        MongoServer *server() const { return _server; }

    protected slots:
        void handle(LoadCollectionNamesResponse *collectionNames);

    private:
        void clearCollections();
        void addCollection(MongoCollection *collection);

    private:
        MongoServer *_server;
        MongoWorker *_client;
        QList<MongoCollection *> _collections;
        QString _name;
        bool _system;

        EventBus *_bus;

    };

    class MongoDatabase_CollectionListLoadedEvent : public Event
    {
        R_EVENT

        MongoDatabase_CollectionListLoadedEvent(QObject *sender, const QList<MongoCollection *> &list) :
            Event(sender),
            collections(list) { }

        QList<MongoCollection *> collections;
    };
}
