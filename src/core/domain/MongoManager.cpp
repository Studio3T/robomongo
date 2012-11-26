#include <QUuid>
#include "MongoManager.h"
#include "MongoServer.h"
#include "mongodb/MongoException.h"
#include "AppRegistry.h"
#include "Dispatcher.h"
#include "mongo/client/dbclient.h"
#include "settings/ConnectionRecord.h"

using namespace Robomongo;

MongoManager::MongoManager(Dispatcher *dispatcher, QObject *parent) :
    QObject(parent),
    _dispatcher(dispatcher) {}

/**
 * @brief Connect to MongoDB server
 */
void MongoManager::connectToServer(const ConnectionRecordPtr &connectionRecord)
{
    MongoServerPtr server(new MongoServer(connectionRecord));
    _servers.append(server);

    _dispatcher->publish(this, new ConnectingEvent(server));
    server->tryConnect();
}





// NOTEBOOK*******************************************************
//        if (connectionRecord->isAuthNeeded())
//            server->authenticate(QString(), connectionRecord->userName(), connectionRecord->userPassword());
