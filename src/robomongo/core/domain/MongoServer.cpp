#include "robomongo/core/domain/MongoServer.h"

#include <QDebug>
#include <QStringList>

#include "robomongo/core/mongodb/MongoException.h"
#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/mongodb/MongoWorker.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"

using namespace Robomongo;
using namespace std;

MongoServer::MongoServer(ConnectionSettings *connectionRecord, bool visible) : QObject(),
    _connectionRecord(connectionRecord),
    _bus(AppRegistry::instance().bus()),
    _visible(visible)
{
    _host = _connectionRecord->serverHost();
    _port = QString::number(_connectionRecord->serverPort());
    _address = QString("%1:%2").arg(_host).arg(_port);

    _connection.reset(new mongo::DBClientConnection);

    _client.reset(new MongoWorker(_bus, _connectionRecord->clone()));

    _bus->send(_client.data(), new InitRequest(this));
    qDebug() << "InitRequest sent";
}

MongoServer::~MongoServer()
{
    clearDatabases();
    delete _connectionRecord;
}

/**
 * @brief Try to connect to MongoDB server.
 * @throws MongoException, if fails
 */
void MongoServer::tryConnect()
{
    _client->send(new EstablishConnectionRequest(this,
        "_connectionRecord->databaseName()",
        "_connectionRecord->userName()",
        "_connectionRecord->userPassword()"));

    qDebug() << "EstablishConnectionRequest sent";
}

void MongoServer::insertDocument(const mongo::BSONObj &obj, const QString &db, const QString &collection)
{
    _client->send(new InsertDocumentRequest(this, obj, db, collection));
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
