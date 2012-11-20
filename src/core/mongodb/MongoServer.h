#ifndef MONGOSERVER_H
#define MONGOSERVER_H

#include <QObject>
#include "mongo/client/dbclient.h"
#include "Core.h"
#include <QList>
#include <QHash>

namespace Robomongo
{
    class MongoServer : public QObject
    {
        Q_OBJECT
    public:
        explicit MongoServer(const ConnectionRecordPtr &connectionRecord);

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
         * @brief Load list of all database names
         */
        QStringList databaseNames();

        /**
         * @brief Returns last error message
         */
        QString lastErrorMessage() { return _lastErrorMessage; }

        /**
         * @brief Returns associated connection record
         */
        const ConnectionRecordPtr connectionRecord() const { return _connectionRecord; }


        const QList<MongoDatabasePtr> listDatabases();


    private:

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
