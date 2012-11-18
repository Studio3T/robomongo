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
        explicit MongoManager(QObject *parent = 0);

        /**
         * @brief Connect to MongoDB server
         */
        MongoServerPtr connectToServer(const QString & host, const QString & port, const QString & database,
                                       const QString & username, const QString & password);

    };
}

#endif // MONGOMANAGER_H
