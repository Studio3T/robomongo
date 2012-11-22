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

    signals:

        void databaseListLoaded(const QList<MongoDatabasePtr> &list);
        void connectionEstablished(const MongoServerPtr &server, const QString &address);
        void connectionFailed(const MongoServerPtr &server, const QString &address);

    public slots:
        void onDatabaseNameLoaded(const QStringList &names);
        void onConnectionEstablished(const QString &address);
        void onConnectionFailed(const QString &address);



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

    };
}

#endif // MONGOSERVER_H
