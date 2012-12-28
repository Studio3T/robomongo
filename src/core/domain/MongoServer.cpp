#include <QStringList>
#include "MongoServer.h"
#include "mongodb/MongoException.h"
#include "MongoDatabase.h"
#include "settings/ConnectionRecord.h"
#include "mongodb/MongoClient.h"
#include "AppRegistry.h"
#include "EventBus.h"

using namespace Robomongo;
using namespace std;

MongoServer::MongoServer(ConnectionRecord *connectionRecord, bool visible, const QString &defaultDatabase) : QObject(),
    _connectionRecord(connectionRecord->clone()), // clone connection record
    _bus(AppRegistry::instance().bus()),
    _visible(visible),
    _defaultDatabase(defaultDatabase)
{
    _host = _connectionRecord->databaseAddress();
    _port = QString::number(_connectionRecord->databasePort());
    _address = QString("%1:%2").arg(_host).arg(_port);

    _connection.reset(new mongo::DBClientConnection);

    _client.reset(new MongoClient(
                      _bus,
                      connectionRecord->databaseAddress(),
                      connectionRecord->databasePort(),
                      connectionRecord->databaseName(),
                      connectionRecord->userName(),
                      connectionRecord->userPassword(),
                      _defaultDatabase));

    _bus->send(_client.data(), new InitRequest(this));
}

MongoServer::~MongoServer()
{
    clearDatabases();
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

void MongoServer::loadDatabases()
{
    _client->send(new LoadDatabaseNamesRequest(this));
}

void MongoServer::clearDatabases()
{
    qDeleteAll(_databases);
    _databases.clear();
}

void MongoServer::addDatabase(MongoDatabase *database)
{
    _databases.append(database);
}

void MongoServer::handle(EstablishConnectionResponse *event)
{
    if (event->isError())
    {
        _bus->publish(new ConnectionFailedEvent(this));
        return;
    }

    if (_visible)
        _bus->publish(new ConnectionEstablishedEvent(this));
}

void MongoServer::handle(LoadDatabaseNamesResponse *event)
{
    if (event->isError())
    {
        _bus->publish(new ConnectionFailedEvent(this));
        return;
    }

    clearDatabases();
    foreach(QString name, event->databaseNames)
    {
        MongoDatabase *db  = new MongoDatabase(this, name);
        addDatabase(db);
    }

    _bus->publish(new DatabaseListLoadedEvent(this, _databases));
}
