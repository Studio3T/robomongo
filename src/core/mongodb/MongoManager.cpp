#include <QUuid>
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
void MongoManager::connectToServer(const ConnectionRecordPtr &connectionRecord)
{
    QUuid uid = QUuid::createUuid();
    MongoServerPtr server(new MongoServer(connectionRecord));
    connect(server.get(), SIGNAL(connectionEstablished(MongoServerPtr, QString)), this, SLOT(onConnectionEstablished(MongoServerPtr, QString)));
    connect(server.get(), SIGNAL(connectionFailed(MongoServerPtr, QString)), this, SLOT(onConnectionFailed(MongoServerPtr, QString)));
    _servers.append(server);

    emit connecting(server);
    server->tryConnect();
}


void MongoManager::onConnectionEstablished(const MongoServerPtr &server, const QString &address)
{
    emit connected(server);
}

void MongoManager::onConnectionFailed(const MongoServerPtr &server, const QString &address)
{
    emit connectionFailed(server);
}



// NOTEBOOK*******************************************************
//        if (connectionRecord->isAuthNeeded())
//            server->authenticate(QString(), connectionRecord->userName(), connectionRecord->userPassword());
