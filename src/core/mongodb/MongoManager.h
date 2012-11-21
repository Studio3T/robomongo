#ifndef MONGOMANAGER_H
#define MONGOMANAGER_H

#include <QObject>
#include "Core.h"

namespace Robomongo
{
    class MongoManager : public QObject
    {
        Q_OBJECT

    public:
        MongoManager(QObject *parent = 0);

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

        void connectionFailed(const ConnectionRecordPtr &connection);

        /**
         * @brief Fires when disconnected
         */
        void disconnected(const MongoServerPtr &server);
    };
}

#endif // MONGOMANAGER_H
