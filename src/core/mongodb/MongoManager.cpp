#include "MongoManager.h"
#include "MongoServer.h"
#include "MongoException.h"
#include "mongo/client/dbclient.h"
#include "settings/ConnectionRecord.h"

using namespace Robomongo;

MongoManager::MongoManager(QObject *parent) :
    QObject(parent)
{
}

/**
 * @brief Connect to MongoDB server
 */
MongoServerPtr MongoManager::connectToServer(const ConnectionRecordPtr &connectionRecord)
{
    MongoServerPtr server(new MongoServer(connectionRecord));
    server->tryConnect();

    if (connectionRecord->isAuthNeeded())
        server->authenticate(QString(), connectionRecord->userName(), connectionRecord->userPassword());

    emit connected(server);
    return server;
}
