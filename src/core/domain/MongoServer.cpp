#include <QStringList>
#include "MongoServer.h"
#include "mongodb/MongoException.h"
#include "MongoDatabase.h"
#include "settings/ConnectionRecord.h"
#include "mongodb/MongoClient.h"
#include "AppRegistry.h"
#include "Dispatcher.h"

using namespace Robomongo;
using namespace std;

MongoServer::MongoServer(const ConnectionRecordPtr &connectionRecord, bool visible) : QObject(),
    _connectionRecord(connectionRecord),
    _dispatcher(AppRegistry::instance().dispatcher()),
    _visible(visible)
{
    _host = _connectionRecord->databaseAddress();
    _port = QString::number(_connectionRecord->databasePort());
    _address = QString("%1:%2").arg(_host).arg(_port);

    _connection.reset(new mongo::DBClientConnection);

    _client.reset(new MongoClient(
                      connectionRecord->databaseAddress(),
                      connectionRecord->databasePort(),
                      connectionRecord->databaseName(),
                      connectionRecord->userName(),
                      connectionRecord->userPassword()));

    _client->send(new InitRequest(this));
}

MongoServer::~MongoServer()
{
    int z = 80;
}

/**
 * @brief Try to connect to MongoDB server.
 * @throws MongoException, if fails
 */
void MongoServer::tryConnect()
{
    _client->send(new EstablishConnectionRequest(this,
        _connectionRecord->databaseName(),
        _connectionRecord->userName(),
        _connectionRecord->userPassword()));
}

/**
 * @brief Try to connect to MongoDB server.
 * @throws MongoException, if fails
 */
bool MongoServer::authenticate(const QString &database, const QString &username, const QString &password)
{
    std::string errmsg;
    bool ok = _connection->auth(database.toStdString(), username.toStdString(), password.toStdString(), errmsg);

    if (!ok)
    {
        _lastErrorMessage = QString::fromStdString(errmsg);
        throw MongoException("Unable to authorize");
    }
}

void MongoServer::listDatabases()
{
    _client->send(new LoadDatabaseNamesRequest(this));
}

void MongoServer::handle(LoadDatabaseNamesResponse *event)
{
    if (event->isError())
    {
        _dispatcher.publish(new ConnectionFailedEvent(this, shared_from_this()));
        return;
    }

    QList<MongoDatabasePtr> list;

    foreach(QString name, event->databaseNames)
    {
        MongoDatabasePtr db(new MongoDatabase(this, name));
        list.append(db);
    }

    _dispatcher.publish(new DatabaseListLoadedEvent(this, list));
}

void MongoServer::handle(EstablishConnectionResponse *event)
{
    if (event->isError())
    {
        _dispatcher.publish(new ConnectionFailedEvent(this, shared_from_this()));
        return;
    }

    if (_visible)
        _dispatcher.publish(new ConnectionEstablishedEvent(this, shared_from_this()));
}
