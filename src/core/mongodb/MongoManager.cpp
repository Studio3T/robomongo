#include "MongoManager.h"
#include "MongoServer.h"
#include "MongoException.h"
#include "mongo/client/dbclient.h"
#include "settings/ConnectionRecord.h"

using namespace Robomongo;

MongoManager::MongoManager(QObject *parent) : QObject(parent)
{
}

MongoManager::~MongoManager()
{

}

/**
 * @brief Connect to MongoDB server
 */
MongoServerPtr MongoManager::connectToServer(const ConnectionRecordPtr &connectionRecord)
{
    MongoServerPtr server(new MongoServer(connectionRecord));
    connect(server.data(), SIGNAL(connectionEstablished(QString)), this, SLOT(onConnectionEstablished(QString)));
    connect(server.data(), SIGNAL(connectionFailed(QString)), this, SLOT(onConnectionFailed(QString)));

    _servers.insert(connectionRecord->getFullAddress(), server);
    server->tryConnect();

    return server;
}


void MongoManager::onConnectionEstablished(const QString &address)
{
    emit connected(_servers.value(address));
}

void MongoManager::onConnectionFailed(const QString &address)
{
    emit connectionFailed(_servers.value(address));
}



// NOTEBOOK*******************************************************
//        if (connectionRecord->isAuthNeeded())
//            server->authenticate(QString(), connectionRecord->userName(), connectionRecord->userPassword());
