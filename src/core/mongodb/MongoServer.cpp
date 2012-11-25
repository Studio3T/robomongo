#include <QStringList>
#include "MongoServer.h"
#include "MongoException.h"
#include "MongoDatabase.h"
#include "settings/ConnectionRecord.h"
#include "MongoClient.h"
#include "AppRegistry.h"
#include "Dispatcher.h"

using namespace Robomongo;
using namespace std;

MongoServer::MongoServer(const ConnectionRecordPtr &connectionRecord) : QObject(),
    _connectionRecord(connectionRecord),
    _dispatcher(AppRegistry::instance().dispatcher())
{
    _host = _connectionRecord->databaseAddress();
    _port = QString::number(_connectionRecord->databasePort());
    _address = QString("%1:%2").arg(_host).arg(_port);

    _connection.reset(new mongo::DBClientConnection);

    _client.reset(new MongoClient(_address));
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
    _client->send(new EstablishConnectionRequest(this));
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

/**
 * @brief Events dispatcher
 */
bool MongoServer::event(QEvent * event)
{
    R_HANDLE(event) {
        R_EVENT(EstablishConnectionResponse);
        R_EVENT(LoadDatabaseNamesResponse);
    }

    return QObject::event(event);
}

void MongoServer::handle(const LoadDatabaseNamesResponse *event)
{
    if (event->isError())
    {
        _dispatcher.publish(this, new ConnectionFailedEvent(shared_from_this()));
        return;
    }

    QList<MongoDatabasePtr> list;

    foreach(QString name, event->databaseNames)
    {
        MongoDatabasePtr db(new MongoDatabase(this, name));
        list.append(db);
    }

    _dispatcher.publish(this, new DatabaseListLoadedEvent(list));
}

void MongoServer::handle(const EstablishConnectionResponse *event)
{
    if (event->isError())
    {
        _dispatcher.publish(this, new ConnectionFailedEvent(shared_from_this()));
        return;
    }

    _dispatcher.publish(this, new ConnectionEstablishedEvent(shared_from_this()));
}
