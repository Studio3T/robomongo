#ifndef MONGOSERVER_H
#define MONGOSERVER_H

#include <QObject>
#include "mongo/client/dbclient.h"
#include "Core.h"

namespace Robomongo
{
    class MongoServer : public QObject
    {
        Q_OBJECT
    public:
        explicit MongoServer(const QString &host, const QString &port);

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


    private:

        DBClientConnection_ScopedPtr _connection;

        QString _host;
        QString _port;
        QString _address;
        QString _lastErrorMessage;
    };
}

#endif // MONGOSERVER_H
