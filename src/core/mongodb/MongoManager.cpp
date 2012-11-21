#include "MongoManager.h"
#include "MongoServer.h"
#include "MongoException.h"
#include "mongo/client/dbclient.h"
#include "settings/ConnectionRecord.h"

using namespace Robomongo;

MongoManager::MongoManager(QObject *parent) : QObject(parent), Wrapper(this)
{
}

/**
 * @brief Connect to MongoDB server
 */
MongoServerPtr MongoManager::connectToServer(const ConnectionRecordPtr &connectionRecord)
{
    try
    {
        MongoServerPtr server(new MongoServer(connectionRecord));
        server->tryConnect();

        //sleep(1000);

        if (connectionRecord->isAuthNeeded())
            server->authenticate(QString(), connectionRecord->userName(), connectionRecord->userPassword());

        emit connected(server);
        return server;
    }
    catch(MongoException &ex)
    {
        emit connectionFailed(connectionRecord);
//        QString message = QString("Cannot connect to MongoDB (%1)").arg(selected->getFullAddress());
//        QMessageBox::information(this, "Error", message);
    }
}
