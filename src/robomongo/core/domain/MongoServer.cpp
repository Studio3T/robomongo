#include "robomongo/core/domain/MongoServer.h"

#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/mongodb/MongoWorker.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    R_REGISTER_EVENT(MongoServerLoadingDatabasesEvent)

    MongoServer::MongoServer(ConnectionSettings *connectionRecord, bool visible) : QObject(),
        _version(0.0f),
        _visible(visible),
        _client(new MongoWorker(connectionRecord->clone(),AppRegistry::instance().settingsManager()->loadMongoRcJs(),AppRegistry::instance().settingsManager()->batchSize())),
        _isConnected(false)
    {
    }

    bool MongoServer::isConnected() const
    {
        return _isConnected;
    }

    ConnectionSettings *MongoServer::connectionRecord() const 
    { 
        return _client->connectionRecord(); 
    }

    MongoServer::~MongoServer()
    {        
        clearDatabases();
        delete _client;
    }

    /**
     * @brief Try to connect to MongoDB server.
     * @throws MongoException, if fails
     */
    void MongoServer::tryConnect()
    {
        AppRegistry::instance().bus()->send(_client,new EstablishConnectionRequest(this,
            "_connectionRecord->databaseName()",
            "_connectionRecord->userName()",
            "_connectionRecord->userPassword()"));
    }

    QStringList MongoServer::getDatabasesNames() const
    {
        QStringList result;
        for (DatabasesContainerType::const_iterator it = _databases.begin(); it != _databases.end(); ++it) {
            MongoDatabase *datab = *it;
            result.append(QtUtils::toQString(datab->name()));
        }
        return result;
    }

    void MongoServer::createDatabase(const std::string &dbName)
    {
        AppRegistry::instance().bus()->send(_client,new CreateDatabaseRequest(this, dbName));
    }

    MongoDatabase *MongoServer::findDatabaseByName(const std::string &dbName) const
    {
        for (DatabasesContainerType::const_iterator it = _databases.begin(); it != _databases.end(); ++it) {
            MongoDatabase *datab = *it;
            if (datab->name() == dbName) {
                return datab;
            }
        }
        return NULL;
    }

    void MongoServer::dropDatabase(const std::string &dbName)
    {
        AppRegistry::instance().bus()->send(_client,new DropDatabaseRequest(this, dbName));
    }

    void MongoServer::insertDocuments(const std::vector<mongo::BSONObj> &objCont, const MongoNamespace &ns)
    {
        for (std::vector<mongo::BSONObj>::const_iterator it = objCont.begin(); it != objCont.end(); it++) {
            insertDocument(*it, ns);
        }
    }

    void MongoServer::insertDocument(const mongo::BSONObj &obj, const MongoNamespace &ns)
    {
        AppRegistry::instance().bus()->send(_client,new InsertDocumentRequest(this, obj, ns));
    }

    void MongoServer::saveDocuments(const std::vector<mongo::BSONObj> &objCont, const MongoNamespace &ns)
    {
        for (std::vector<mongo::BSONObj>::const_iterator it = objCont.begin(); it != objCont.end(); it++) {
            saveDocument(*it,ns);
        }
    }

    void MongoServer::saveDocument(const mongo::BSONObj &obj, const MongoNamespace &ns)
    {
        AppRegistry::instance().bus()->send(_client, new InsertDocumentRequest(this, obj, ns, true));
    }

    void MongoServer::removeDocuments(mongo::Query query, const MongoNamespace &ns, bool justOne)
    {
        AppRegistry::instance().bus()->send(_client, new RemoveDocumentRequest(this, query, ns, justOne));
    }

    void MongoServer::loadDatabases()
    {
        AppRegistry::instance().bus()->publish(new MongoServerLoadingDatabasesEvent(this));
        AppRegistry::instance().bus()->send(_client, new LoadDatabaseNamesRequest(this));
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
        const ConnectionInfo &info = event->info();
        _isConnected = !event->isError();
        if (event->isError()) {            
            AppRegistry::instance().bus()->publish(new ConnectionFailedEvent(this, event->error()));
        } else if (_visible) {
            AppRegistry::instance().bus()->publish(new ConnectionEstablishedEvent(this));
            clearDatabases();
            for(std::vector<std::string>::const_iterator it = info._databases.begin(); it != info._databases.end(); ++it) {
                const std::string &name = *it;
                MongoDatabase *db  = new MongoDatabase(this, name);
                addDatabase(db);
            }
        }        
        _version = info._version;
    }

    void MongoServer::handle(LoadDatabaseNamesResponse *event)
    {
        if (event->isError()) {
            AppRegistry::instance().bus()->publish(new ConnectionFailedEvent(this, event->error()));
            return;
        }

        clearDatabases();
        for(std::vector<std::string>::iterator it = event->databaseNames.begin(); it != event->databaseNames.end(); ++it) {
            const std::string &name = *it;
            MongoDatabase *db  = new MongoDatabase(this, name);
            addDatabase(db);
        }

        AppRegistry::instance().bus()->publish(new DatabaseListLoadedEvent(this, _databases));
    }

    void MongoServer::handle(InsertDocumentResponse *event)
    {
        AppRegistry::instance().bus()->publish(new InsertDocumentResponse(event->sender(), event->error()));
    }
}
