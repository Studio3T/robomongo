#pragma once

#include <QObject>
#include <QUuid>
#include <QList>
#include <QHash>
#include <QScopedPointer>
#include <mongo/client/dbclient.h>

#include "robomongo/core/Core.h"
#include "robomongo/core/mongodb/MongoWorker.h"

namespace Robomongo
{
    class MongoWorker;
    class MongoDatabase;
    class EstablishConnectionResponse;
    class LoadDatabaseNamesResponse;

    /**
     * @brief MongoServer represents active connection to MongoDB server.
     * MongoServer is an Aggregate Root, that manages three internal entities:
     * MongoDatabase, MongoCollection and MongoWorker.
     */
    class MongoServer : public QObject
    {
        Q_OBJECT

    public:
        /**
         * @brief MongoServer
         * @param connectionRecord: MongoServer will own this ConnectionSettings.
         * @param visible
         * @param defaultDatabase
         */
        MongoServer(ConnectionSettings *connectionRecord, bool visible);
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
        ConnectionSettings *connectionRecord() const { return _connectionRecord; }

        /**
         * @brief Loads databases of this server asynchronously.
         */
        void loadDatabases();

        MongoWorker *client() const { return _client.data(); }

    protected slots:
        void handle(EstablishConnectionResponse *event);
        void handle(LoadDatabaseNamesResponse *event);

    private:
        void clearDatabases();
        void addDatabase(MongoDatabase *database);

        QScopedPointer<MongoWorker> _client;
        DBClientConnection_ScopedPtr _connection;

        /**
         * @brief Associated connection record
         */
        ConnectionSettings *_connectionRecord;

        QString _host;
        QString _port;
        QString _address;
        QString _lastErrorMessage;
        bool _visible;

        QList<MongoDatabase *> _databases;

        EventBus *_bus;
    };
}
