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
    class EstablishConnectionResponse;
    class LoadDatabaseNamesResponse;

    class MongoServer : public QObject
    {
        Q_OBJECT
    public:
        explicit MongoServer(const ConnectionRecordPtr &connectionRecord, bool visible);
        ~MongoServer();

        /**
         * @brief Try to connect to MongoDB server.
         * @throws MongoException, if fails
         */
        void tryConnect();

        /**
         * @brief Try to connect to MongoDB server.
         * @throws MongoException, if fails
         */
        bool authenticate(const QString &database, const QString &username, const QString &password);

        /**
         * @brief Returns last error message
         */
        QString lastErrorMessage() { return _lastErrorMessage; }

        /**
         * @brief Returns associated connection record
         */
        const ConnectionRecordPtr connectionRecord() const { return _connectionRecord; }

        void listDatabases();

        MongoClient *client() const { return _client.data(); }

    protected slots:
        void handle(EstablishConnectionResponse *event);
        void handle(LoadDatabaseNamesResponse *event);

    private:

        QScopedPointer<MongoClient> _client;

        DBClientConnection_ScopedPtr _connection;

        /**
         * @brief Associated connection record
         */
        ConnectionRecordPtr _connectionRecord;

        QString _host;
        QString _port;
        QString _address;
        QString _lastErrorMessage;
        bool _visible;

        QList<MongoDatabasePtr> _databases;
        QHash<QString, MongoDatabasePtr> _databasesByName;
        EventBus &_bus;

    };
}

#endif // MONGOSERVER_H
