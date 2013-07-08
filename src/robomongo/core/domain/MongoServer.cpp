#include "robomongo/core/domain/MongoServer.h"

#include <QDebug>
#include <QStringList>

#include "robomongo/core/mongodb/MongoException.h"
#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/mongodb/MongoWorker.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"


using namespace std;
namespace Robomongo
{
    R_REGISTER_EVENT(MongoServer_LoadingDatabasesEvent)

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
        _connectionRecord=NULL;
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

    void MongoServer::createDatabase(const QString &dbName)
    {
        _client->send(new CreateDatabaseRequest(this, dbName));
    }

    void MongoServer::dropDatabase(const QString &dbName)
    {
        _client->send(new DropDatabaseRequest(this, dbName));
    }

    void MongoServer::insertDocument(const mongo::BSONObj &obj, const QString &db, const QString &collection)
    {
        _client->send(new InsertDocumentRequest(this, obj, db, collection));
    }

    void MongoServer::saveDocument(const mongo::BSONObj &obj, const QString &db, const QString &collection)
    {
        _client->send(new InsertDocumentRequest(this, obj, db, collection, true));
    }

    void MongoServer::removeDocuments(mongo::Query query, const QString &db, const QString &collection, bool justOne)
    {
        _client->send(new RemoveDocumentRequest(this, query, db, collection, justOne));
    }

    void MongoServer::loadDatabases()
    {
        _bus->publish(new MongoServer_LoadingDatabasesEvent(this));
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
        if (event->isError()){
            _bus->publish(new ConnectionFailedEvent(this));
        }else if (_visible){
            _bus->publish(new ConnectionEstablishedEvent(this));
        }
    }

    void MongoServer::handle(LoadDatabaseNamesResponse *event)
    {
        if (event->isError()){
            _bus->publish(new ConnectionFailedEvent(this));
            return;
        }

        clearDatabases();
        for(QStringList::iterator it = event->databaseNames.begin();it!=event->databaseNames.end();++it){
            const QString &name = *it;
            MongoDatabase *db  = new MongoDatabase(this, name);
            addDatabase(db);
        }

        _bus->publish(new DatabaseListLoadedEvent(this, _databases));
    }

    void MongoServer::handle(InsertDocumentResponse *event)
    {
        _bus->publish(new InsertDocumentResponse(event->sender(), event->error()));
    }
}
