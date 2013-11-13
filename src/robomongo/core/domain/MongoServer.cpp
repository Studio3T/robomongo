#include "robomongo/core/domain/MongoServer.h"

#include <QApplication>

#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/mongodb/MongoWorker.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"

namespace Robomongo
{
    MongoServer::MongoServer(IConnectionSettingsBase *connectionRecord, bool visible) : QObject(),
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

    IConnectionSettingsBase *MongoServer::connectionRecord() const 
    { 
        return _client->connectionRecord(); 
    }

    MongoServer::~MongoServer()
    {        
        clearDatabases();
        delete _client;
    }

    void MongoServer::postEventToDataBase(QEvent *event, int priority) const
    {
        qApp->postEvent(_client, event, priority);
    }

    void MongoServer::customEvent(QEvent *event)
    {
        QEvent::Type type = event->type();
        if(type==static_cast<QEvent::Type>(CreateDataBaseEvent::EventType)){
            CreateDataBaseEvent *ev = static_cast<CreateDataBaseEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(DropDatabaseEvent::EventType)){
            DropDatabaseEvent *ev = static_cast<DropDatabaseEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(LoadDatabaseNamesEvent::EventType)){
            LoadDatabaseNamesEvent *ev = static_cast<LoadDatabaseNamesEvent*>(event);
            ErrorInfo er = ev->errorInfo();

            if (!er.isError()) {
                LoadDatabaseNamesEvent::value_type v = ev->value();

                clearDatabases();
                for(std::vector<std::string>::iterator it = v._databaseNames.begin(); it != v._databaseNames.end(); ++it) {
                    const std::string &name = *it;
                    MongoDatabase *db  = new MongoDatabase(this, name);
                    addDatabase(db);
                }
                emit databaseListLoaded(_databases);
            }
        }
        else if(type==static_cast<QEvent::Type>(SaveDocumentEvent::EventType)){
            SaveDocumentEvent *ev = static_cast<SaveDocumentEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(ExecuteQueryEvent::EventType)){
            ExecuteQueryEvent *ev = static_cast<ExecuteQueryEvent*>(event);
            ExecuteQueryEvent::value_type v = ev->value();
            emit documentListLoaded(v);
        }
        else if(type==static_cast<QEvent::Type>(EstablishConnectionEvent::EventType)){
            EstablishConnectionEvent *ev = static_cast<EstablishConnectionEvent*>(event);
            EstablishConnectionEvent::value_type v = ev->value();
            ErrorInfo er = ev->errorInfo();

            _isConnected = !er.isError();
            if (_visible) {
                clearDatabases();
                for(std::vector<std::string>::const_iterator it = v._info._databases.begin(); it != v._info._databases.end(); ++it) {
                    const std::string &name = *it;
                    MongoDatabase *db  = new MongoDatabase(this, name);
                    addDatabase(db);
                }
            }
            emit connectedStatus(er);
            emit finishConnected();
            _version = v._info._version;
        }
        return BaseClass::customEvent(event);
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
    
     /**
     * @brief Try to connect to MongoDB server.
     * @throws MongoException, if fails
     */
    void MongoServer::tryConnect()
    {
        if(!_isConnected){
            emit startConnected();
            EstablishConnectionInfo inf;
            qApp->postEvent(_client, new EstablishConnectionEvent(this, inf));
        }
    }

    void MongoServer::createDatabase(const std::string &dbName)
    {
        CreateDataBaseInfo inf(dbName);
        qApp->postEvent(_client, new CreateDataBaseEvent(this, inf));
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
        DropDatabaseInfo inf(dbName);
        qApp->postEvent(_client, new DropDatabaseEvent(this, inf));
    }

    void MongoServer::insertDocuments(const std::vector<mongo::BSONObj> &objCont, const MongoNamespace &ns)
    {
        for (std::vector<mongo::BSONObj>::const_iterator it = objCont.begin(); it != objCont.end(); it++) {
            insertDocument(*it, ns);
        }
    }

    void MongoServer::insertDocument(const mongo::BSONObj &obj, const MongoNamespace &ns)
    {
        SaveDocumentInfo inf(obj, ns, false);
        qApp->postEvent(_client, new SaveDocumentEvent(this, inf));
    }

    void MongoServer::saveDocuments(const std::vector<mongo::BSONObj> &objCont, const MongoNamespace &ns)
    {
        for (std::vector<mongo::BSONObj>::const_iterator it = objCont.begin(); it != objCont.end(); it++) {
            saveDocument(*it,ns);
        }
    }

    void MongoServer::saveDocument(const mongo::BSONObj &obj, const MongoNamespace &ns)
    {
        SaveDocumentInfo inf(obj, ns, true);
        qApp->postEvent(_client, new SaveDocumentEvent(this, inf));
    }

    void MongoServer::removeDocuments(mongo::Query query, const MongoNamespace &ns, bool justOne)
    {
        RemoveDocumentInfo inf(query, ns, justOne);
        qApp->postEvent(_client, new RemoveDocumentEvent(this, inf));
    }

    void MongoServer::query(int resultIndex, const MongoQueryInfo &info)
    {
        ExecuteQueryInfo inf(resultIndex,info);
        qApp->postEvent(_client, new ExecuteQueryEvent(this, inf));
    }

    void MongoServer::loadDatabases()
    {
        emit startedLoadDatabases();
        LoadDatabaseNamesInfo inf;
        qApp->postEvent(_client, new LoadDatabaseNamesEvent(this, inf));
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
}
