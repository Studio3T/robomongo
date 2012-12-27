#ifndef MONGOSERVER_H
#define MONGOSERVER_H

#include <QObject>
#include <QUuid>
#include "mongo/client/dbclient.h"
#include "Core.h"
#include <QList>
#include <QHash>
#include <QScopedPointer>
#include "mongodb/MongoClient.h"

namespace Robomongo
{
    class MongoClient;
    class MongoDatabase;
    class EstablishConnectionResponse;
    class LoadDatabaseNamesResponse;

    class MongoServer : public QObject
    {
        Q_OBJECT
    public:
        MongoServer(ConnectionRecord *connectionRecord, bool visible, MongoDatabase *defaultDatabase = NULL);
        ~MongoServer();

        /**
         * @brief Try to connect to MongoDB server.
         * @throws MongoException, if fails
         */
        void tryConnect();

        /**
         * @brief Returns last error message
         */
        QString lastErrorMessage() { return _lastErrorMessage; }

        /**
         * @brief Returns associated connection record
         */
        ConnectionRecord *connectionRecord() const { return _connectionRecord; }



        /**
         * @brief Loads databases of this server asynchronously.
         */
        void loadDatabases();

        MongoClient *client() const { return _client.data(); }

    protected slots:
        void handle(EstablishConnectionResponse *event);
        void handle(LoadDatabaseNamesResponse *event);

    private:
        void clearDatabases();
        void addDatabase(MongoDatabase *database);

    private:

        QScopedPointer<MongoClient> _client;
        DBClientConnection_ScopedPtr _connection;

        /**
         * @brief Associated connection record
         */
        ConnectionRecord *_connectionRecord;

        QString _host;
        QString _port;
        QString _address;
        QString _lastErrorMessage;
        bool _visible;

        QList<MongoDatabase *> _databases;

        EventBus *_bus;
        MongoDatabase *_defaultDatabase;
    };
}

#endif // MONGOSERVER_H
