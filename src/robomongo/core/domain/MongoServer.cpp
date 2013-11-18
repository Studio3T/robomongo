#include "robomongo/core/domain/MongoServer.h"

#include <QApplication>

#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/mongodb/MongoWorker.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/events/MongoEvents.hpp"

namespace Robomongo
{
    MongoServer::MongoServer(const IConnectionSettingsBase *connectionRecord, bool visible) : QObject(),
        _version(0.0f),
        _visible(visible),
        _client(new MongoWorker(connectionRecord->clone(),AppRegistry::instance().settingsManager()->loadMongoRcJs(),AppRegistry::instance().settingsManager()->batchSize()))
    {
    }

    bool MongoServer::isConnected() const
    {
        return _client->isConnected();
    }

    const IConnectionSettingsBase *MongoServer::connectionRecord() const 
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
        if(type==static_cast<QEvent::Type>(Events::CreateDataBaseResponceEvent::EventType)){
            Events::CreateDataBaseResponceEvent *ev = static_cast<Events::CreateDataBaseResponceEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::DropDatabaseResponceEvent::EventType)){
            Events::DropDatabaseResponceEvent *ev = static_cast<Events::DropDatabaseResponceEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::LoadDatabaseNamesResponceEvent::EventType)){
            Events::LoadDatabaseNamesResponceEvent *ev = static_cast<Events::LoadDatabaseNamesResponceEvent*>(event);
            Events::LoadDatabaseNamesResponceEvent::value_type v = ev->value();

            refreshDatabase(v._databases);
            emit finishedLoadDatabases(v);
        }
        else if(type==static_cast<QEvent::Type>(Events::SaveDocumentResponceEvent::EventType)){
            Events::SaveDocumentResponceEvent *ev = static_cast<Events::SaveDocumentResponceEvent*>(event);
            Events::SaveDocumentResponceEvent::value_type v = ev->value();
            emit finishedInsertDocument(v);
        }
        else if(type==static_cast<QEvent::Type>(Events::RemoveDocumentResponceEvent::EventType)){
            Events::RemoveDocumentResponceEvent *ev = static_cast<Events::RemoveDocumentResponceEvent*>(event);
            Events::RemoveDocumentResponceEvent::value_type v = ev->value();
            emit finishedRemoveDocument(v);
        }
        else if(type==static_cast<QEvent::Type>(Events::ExecuteQueryResponceEvent::EventType)){
            Events::ExecuteQueryResponceEvent *ev = static_cast<Events::ExecuteQueryResponceEvent*>(event);
            Events::ExecuteQueryResponceEvent::value_type v = ev->value();
            emit finishedDocumentListLoad(v);
        }
        else if(type==static_cast<QEvent::Type>(Events::EstablishConnectionResponceEvent::EventType)){
            Events::EstablishConnectionResponceEvent *ev = static_cast<Events::EstablishConnectionResponceEvent*>(event);
            Events::EstablishConnectionResponceEvent::value_type v = ev->value();
            ErrorInfo er = v.errorInfo();

            if (_visible) {
                refreshDatabase(v._info._databases);
            }
            _version = v._info._version;
            emit finishedConnect(v);            
        }
        return BaseClass::customEvent(event);
    }

    void MongoServer::refreshDatabase(const std::vector<std::string> &dbs)
    {
        clearDatabases();
        for(std::vector<std::string>::const_iterator it = dbs.begin(); it != dbs.end(); ++it) {
            const std::string &name = *it;
            MongoDatabase *database  = new MongoDatabase(this, name);
             _databases.append(database);
        }
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
        if(!isConnected()){            
            EventsInfo::EstablishConnectionRequestInfo inf;
            emit startedConnect(inf);
            qApp->postEvent(_client, new Events::EstablishConnectionRequestEvent(this, inf));
        }
    }

    void MongoServer::createDatabase(const std::string &dbName)
    {
        EventsInfo::CreateDataBaseInfo inf(dbName);
        qApp->postEvent(_client, new Events::CreateDataBaseRequestEvent(this, inf));
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
        qApp->postEvent(_client, new Events::DropDatabaseRequestEvent(this, inf));
    }

    void MongoServer::insertDocuments(const std::vector<mongo::BSONObj> &objCont, const MongoNamespace &ns)
    {
        for (std::vector<mongo::BSONObj>::const_iterator it = objCont.begin(); it != objCont.end(); it++) {
            insertDocument(*it, ns);
        }
    }
    
    void MongoServer::insertDocument(const mongo::BSONObj &obj, const MongoNamespace &ns)
    {
        EventsInfo::SaveDocumentInfo inf(obj, ns, false);
        emit startedInsertDocument(inf);
        qApp->postEvent(_client, new Events::SaveDocumentRequestEvent(this, inf));
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
        emit startedInsertDocument(inf);
        qApp->postEvent(_client, new Events::SaveDocumentRequestEvent(this, inf));
    }

    void MongoServer::removeDocuments(mongo::Query query, const MongoNamespace &ns, bool justOne)
    {
        EventsInfo::RemoveDocumenInfo inf(query, ns, justOne);
        emit startedRemoveDocument(inf);
        qApp->postEvent(_client, new Events::RemoveDocumentRequestEvent(this, inf));
    }

    void MongoServer::query(int resultIndex, const MongoQueryInfo &info)
    {
        EventsInfo::ExecuteQueryRequestInfo inf(resultIndex,info);
        qApp->postEvent(_client, new Events::ExecuteQueryRequestEvent(this, inf));
    }

    void MongoServer::loadDatabases()
    {
        EventsInfo::LoadDatabaseNamesRequestInfo inf;
        emit startedLoadDatabases(inf);
        qApp->postEvent(_client, new Events::LoadDatabaseNamesRequestEvent(this, inf));
    }

    void MongoServer::clearDatabases()
    {
        qDeleteAll(_databases);
        _databases.clear();
    }
}
