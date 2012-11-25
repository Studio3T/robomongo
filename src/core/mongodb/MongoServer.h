#ifndef MONGOSERVER_H
#define MONGOSERVER_H

#include <QObject>
#include <QUuid>
#include "mongo/client/dbclient.h"
#include "Core.h"
#include <QList>
#include <QHash>
#include <QScopedPointer>
#include "MongoClient.h"
#include "events/MongoEvents.h"

namespace Robomongo
{
    class MongoServer : public QObject, public boost::enable_shared_from_this<MongoServer>
    {
        Q_OBJECT
    public:
        explicit MongoServer(const ConnectionRecordPtr &connectionRecord);
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

        /**
         * @brief Events dispatcher
         */
        virtual bool event(QEvent *event);

    private:
        void handle(const EstablishConnectionResponse *event);
        void handle(const LoadDatabaseNamesResponse *event);

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

        QList<MongoDatabasePtr> _databases;
        QHash<QString, MongoDatabasePtr> _databasesByName;
        Dispatcher &_dispatcher;

    };
}

#endif // MONGOSERVER_H
