#ifndef MONGOMANAGER_H
#define MONGOMANAGER_H

#include <QObject>
#include <QHash>
#include "Core.h"

namespace Robomongo
{
    class MongoManager : public QObject
    {
        Q_OBJECT

    public:
        MongoManager(QObject *parent = 0);
        ~MongoManager();

    public slots:
        /**
         * @brief Connect to MongoDB server
         */
        MongoServerPtr connectToServer(const ConnectionRecordPtr &connectionRecord);

    signals:

        /**
         * @brief Fires when connected
         */
        void connected(const MongoServerPtr &server);

        void connectionFailed(const MongoServerPtr &server);

        /**
         * @brief Fires when disconnected
         */
        void disconnected(const MongoServerPtr &server);

    private slots:
        void onConnectionEstablished(const QString &address);
        void onConnectionFailed(const QString &address);

    private:
        QHash<QString, MongoServerPtr> _servers;
    };
}

#endif // MONGOMANAGER_H
