#include "MongoManager.h"
#include "MongoServer.h"
#include "MongoException.h"
#include "mongo/client/dbclient.h"

using namespace Robomongo;

MongoManager::MongoManager(QObject *parent) :
    QObject(parent)
{
}

MongoServerPtr MongoManager::connectToServer(const QString &host, const QString &port, const QString &database,
                                             const QString &username, const QString &password)
{
    MongoServerPtr server(new MongoServer(host, port));
    server->tryConnect();

    bool userSpecified = !username.isEmpty();
    bool passwordSpecified = !password.isEmpty();

    if (userSpecified || passwordSpecified)
        server->authenticate(database, username, password);
}
