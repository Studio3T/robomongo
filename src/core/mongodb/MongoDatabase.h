#ifndef MONGODATABASE_H
#define MONGODATABASE_H

#include <QObject>
#include "Core.h"
#include "mongo/client/dbclient.h"
#include "boost/shared_ptr.hpp"
#include "events/MongoEvents.h"
#include "MongoClient.h"
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
         * @param server - pointer to parent MongoServer
         */
        MongoDatabase(MongoServer *server, const QString &name);
        ~MongoDatabase();

        void listCollections();

        QString name() const { return _name; }

        /**
         * @brief Checks that this is a system database
         * @return true if system, false otherwise
         */
        bool isSystem() const { return _system; }

        /**
         * @brief Events dispatcher
         */
        virtual bool event(QEvent *);


    signals:

        void collectionListLoaded(const QList<MongoCollectionPtr> &list);

    private:

        void handle(const LoadCollectionNamesResponse *collectionNames);

        MongoServer *_server;
        MongoClient *_client;
        QString _name;
        bool _system;

        Dispatcher &_dispatcher;

    };

}

#endif // MONGODATABASE_H
