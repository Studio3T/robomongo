#include "robomongo/core/domain/MongoServer.h"

#include <QApplication>

#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/mongodb/MongoWorker.h"
#include "robomongo/core/AppRegistry.h"

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
        if(type==static_cast<QEvent::Type>(Events::CreateDataBaseEvent::EventType)){
            Events::CreateDataBaseEvent *ev = static_cast<Events::CreateDataBaseEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::DropDatabaseEvent::EventType)){
            Events::DropDatabaseEvent *ev = static_cast<Events::DropDatabaseEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::LoadDatabaseNamesEvent::EventType)){
            Events::LoadDatabaseNamesEvent *ev = static_cast<Events::LoadDatabaseNamesEvent*>(event);
            Events::LoadDatabaseNamesEvent::value_type inf = ev->value();
            ErrorInfo er = inf.errorInfo();

            if (!er.isError()) {
                Events::LoadDatabaseNamesEvent::value_type v = ev->value();

                clearDatabases();
                for(std::vector<std::string>::iterator it = v._databaseNames.begin(); it != v._databaseNames.end(); ++it) {
                    const std::string &name = *it;
                    MongoDatabase *db  = new MongoDatabase(this, name);
                    addDatabase(db);
                }
                emit databaseListLoaded(_databases);
            }
        }
        else if(type==static_cast<QEvent::Type>(Events::SaveDocumentEvent::EventType)){
            Events::SaveDocumentEvent *ev = static_cast<Events::SaveDocumentEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::ExecuteQueryEvent::EventType)){
            Events::ExecuteQueryEvent *ev = static_cast<Events::ExecuteQueryEvent*>(event);
            Events::ExecuteQueryEvent::value_type v = ev->value();
            emit documentListLoaded(v);
        }
        else if(type==static_cast<QEvent::Type>(Events::EstablishConnectionEvent::EventType)){
            Events::EstablishConnectionEvent *ev = static_cast<Events::EstablishConnectionEvent*>(event);
            Events::EstablishConnectionEvent::value_type inf = ev->value();
            ErrorInfo er = inf.errorInfo();

            _isConnected = !er.isError();
            if (_visible) {
                clearDatabases();
                for(std::vector<std::string>::const_iterator it = inf._info._databases.begin(); it != inf._info._databases.end(); ++it) {
                    const std::string &name = *it;
                    MongoDatabase *db  = new MongoDatabase(this, name);
                    addDatabase(db);
                }
            }
            emit finishConnected(inf);
            _version = inf._info._version;
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
            EventsInfo::EstablishConnectionInfo inf;
            emit startConnected(inf);
            qApp->postEvent(_client, new Events::EstablishConnectionEvent(this, inf));
        }
    }

    void MongoServer::createDatabase(const std::string &dbName)
    {
        EventsInfo::CreateDataBaseInfo inf(dbName);
        qApp->postEvent(_client, new Events::CreateDataBaseEvent(this, inf));
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
        EventsInfo::DropDatabaseInfo inf(dbName);
        qApp->postEvent(_client, new Events::DropDatabaseEvent(this, inf));
    }

    void MongoServer::insertDocuments(const std::vector<mongo::BSONObj> &objCont, const MongoNamespace &ns)
    {
        for (std::vector<mongo::BSONObj>::const_iterator it = objCont.begin(); it != objCont.end(); it++) {
            insertDocument(*it, ns);
        }
    }

    void MongoServer::copyCollectionToDiffServer(const EventsInfo::CopyCollectionToDiffServerInfo &inf)
    {
        qApp->postEvent(_client, new Events::CopyCollectionToDiffServerEvent(this, inf));
    }
    
    void MongoServer::insertDocument(const mongo::BSONObj &obj, const MongoNamespace &ns)
    {
        EventsInfo::SaveDocumentInfo inf(obj, ns, false);
        qApp->postEvent(_client, new Events::SaveDocumentEvent(this, inf));
    }

    void MongoServer::saveDocuments(const std::vector<mongo::BSONObj> &objCont, const MongoNamespace &ns)
    {
        for (std::vector<mongo::BSONObj>::const_iterator it = objCont.begin(); it != objCont.end(); it++) {
            saveDocument(*it,ns);
        }
    }

    void MongoServer::saveDocument(const mongo::BSONObj &obj, const MongoNamespace &ns)
    {
        EventsInfo::SaveDocumentInfo inf(obj, ns, true);
        qApp->postEvent(_client, new Events::SaveDocumentEvent(this, inf));
    }

    void MongoServer::removeDocuments(mongo::Query query, const MongoNamespace &ns, bool justOne)
    {
        EventsInfo::RemoveDocumentInfo inf(query, ns, justOne);
        qApp->postEvent(_client, new Events::RemoveDocumentEvent(this, inf));
    }

    void MongoServer::query(int resultIndex, const MongoQueryInfo &info)
    {
        EventsInfo::ExecuteQueryInfo inf(resultIndex,info);
        qApp->postEvent(_client, new Events::ExecuteQueryEvent(this, inf));
    }

    void MongoServer::loadDatabases()
    {
        emit startedLoadDatabases();
        EventsInfo::LoadDatabaseNamesInfo inf;
        qApp->postEvent(_client, new Events::LoadDatabaseNamesEvent(this, inf));
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
