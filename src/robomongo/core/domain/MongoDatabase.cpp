#include "robomongo/core/domain/MongoDatabase.h"

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/events/MongoEvents.hpp"

namespace Robomongo
{
    MongoDatabase::MongoDatabase(MongoServer *server, const std::string &name) :
        QObject(),
        _system(name == "admin" || name == "local"),
        _server(server),
        _name(name)
    {
    }

    MongoDatabase::~MongoDatabase()
    {
        clearCollections();
    }

    void MongoDatabase::loadCollections()
    {
        EventsInfo::LoadCollectionRequestInfo inf(_name);
        emit startedCollectionListLoad(inf);   
        _server->postEventToDataBase(new Events::LoadCollectionRequestEvent(this, inf));
    }

    void MongoDatabase::loadUsers()
    {
        EventsInfo::LoadUserRequestInfo inf(_name);
        emit startedUserListLoad(inf);
        _server->postEventToDataBase(new Events::LoadUserRequestEvent(this, inf));
    }

    void MongoDatabase::loadFunctions()
    {
        EventsInfo::LoadFunctionRequestInfo inf(_name);
        emit startedFunctionListLoad(inf);
        _server->postEventToDataBase(new Events::LoadFunctionRequestEvent(this, inf));
    }

    void MongoDatabase::createCollection(const std::string &collection)
    {
        EventsInfo::CreateCollectionInfo inf(MongoNamespace(_name, collection));
        _server->postEventToDataBase(new Events::CreateCollectionRequestEvent(this, inf));
    }

    void MongoDatabase::dropCollection(const std::string &collection)
    {
        EventsInfo::DropCollectionInfo inf(MongoNamespace(_name, collection));
        _server->postEventToDataBase(new Events::DropCollectionRequestEvent(this, inf));
    }

    void MongoDatabase::renameCollection(const std::string &collection, const std::string &newCollection)
    {
        EventsInfo::RenameCollectionInfo inf(MongoNamespace(_name, collection), newCollection);
        _server->postEventToDataBase(new Events::RenameCollectionRequestEvent(this, inf));
    }

    void MongoDatabase::duplicateCollection(const std::string &collection, const std::string &newCollection)
    {
        EventsInfo::DuplicateCollectionInfo inf(MongoNamespace(_name, collection), newCollection);
        _server->postEventToDataBase(new Events::DuplicateCollectionRequestEvent(this, inf));
    }

    void MongoDatabase::copyCollection(MongoServer *server, const std::string &sourceDatabase, const std::string &collection)
    {
        EventsInfo::CopyCollectionToDiffServerInfo inf(server, MongoNamespace(sourceDatabase, collection), MongoNamespace(_name, collection));
        _server->postEventToDataBase(new Events::CopyCollectionToDiffServerRequestEvent(this, inf));  
    }

    void MongoDatabase::createUser(const MongoUser &user, bool overwrite)
    {
        EventsInfo::CreateUserInfo inf(_name, user, overwrite);
        _server->postEventToDataBase(new Events::CreateUserRequestEvent(this, inf));
    }

    void MongoDatabase::dropUser(const mongo::OID &id)
    {
        EventsInfo::DropUserInfo inf(_name, id);
        _server->postEventToDataBase(new Events::DropUserRequestEvent(this, inf));
    }

    void MongoDatabase::createFunction(const MongoFunction &fun)
    {       
        EventsInfo::CreateFunctionInfo inf(_name, fun);
        _server->postEventToDataBase(new Events::CreateFunctionRequestEvent(this, inf));
    }

    void MongoDatabase::updateFunction(const std::string &name, const MongoFunction &fun)
    {
        EventsInfo::CreateFunctionInfo inf(_name, fun, name);
        _server->postEventToDataBase(new Events::CreateFunctionRequestEvent(this, inf));
    }

    void MongoDatabase::dropFunction(const std::string &name)
    {
        EventsInfo::DropFunctionInfo inf(_name, name);
        _server->postEventToDataBase(new Events::DropFunctionRequestEvent(this, inf));
    }

    void MongoDatabase::customEvent(QEvent *event)
    {
        QEvent::Type type = event->type();
        if(type==static_cast<QEvent::Type>(Events::DropFunctionResponceEvent::EventType))
        {
            Events::DropFunctionResponceEvent *ev = static_cast<Events::DropFunctionResponceEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::CreateFunctionResponceEvent::EventType))
        {
            Events::CreateFunctionResponceEvent *ev = static_cast<Events::CreateFunctionResponceEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::LoadFunctionResponceEvent::EventType)){
            Events::LoadFunctionResponceEvent *ev = static_cast<Events::LoadFunctionResponceEvent*>(event);
            Events::LoadFunctionResponceEvent::value_type inf = ev->value();
            emit finishedFunctionListLoad(inf);
        }
        else if(type==static_cast<QEvent::Type>(Events::CreateUserResponceEvent::EventType))
        {
            Events::CreateUserResponceEvent *ev = static_cast<Events::CreateUserResponceEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::DropUserResponceEvent::EventType))
        {
            Events::DropUserResponceEvent *ev = static_cast<Events::DropUserResponceEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::LoadUserResponceEvent::EventType)){
            Events::LoadUserResponceEvent *ev = static_cast<Events::LoadUserResponceEvent*>(event);
            Events::LoadUserResponceEvent::value_type inf = ev->value();
            emit finishedUserListLoad(inf);
        }
        else if(type==static_cast<QEvent::Type>(Events::CreateCollectionResponceEvent::EventType))
        {
            Events::CreateCollectionResponceEvent *ev = static_cast<Events::CreateCollectionResponceEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::DropCollectionResponceEvent::EventType))
        {
            Events::DropCollectionResponceEvent *ev = static_cast<Events::DropCollectionResponceEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::RenameCollectionResponceEvent::EventType))
        {
            Events::RenameCollectionResponceEvent *ev = static_cast<Events::RenameCollectionResponceEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::LoadCollectionResponceEvent::EventType)){
            Events::LoadCollectionResponceEvent *ev = static_cast<Events::LoadCollectionResponceEvent*>(event);            
            EventsInfo::LoadCollectionResponceInfo inf = ev->value();
            ErrorInfo er = inf.errorInfo();
            clearCollections();
            if (!er.isError()){
                for (std::vector<MongoCollectionInfo>::const_iterator it = inf._infos.begin(); it != inf._infos.end(); ++it) {
                    const MongoCollectionInfo &info = *it;
                    MongoCollection *collection = new MongoCollection(this, info);
                    _collections.push_back(collection);
                }                
            }
            emit finishedCollectionListLoad(inf);
        }
        else if(type==static_cast<QEvent::Type>(Events::DuplicateCollectionResponceEvent::EventType))
        {
            Events::DuplicateCollectionResponceEvent *ev = static_cast<Events::DuplicateCollectionResponceEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::CopyCollectionToDiffServerResponceEvent::EventType))
        {
            Events::CopyCollectionToDiffServerResponceEvent *ev = static_cast<Events::CopyCollectionToDiffServerResponceEvent*>(event);
        }
    }

    void MongoDatabase::clearCollections()
    {
        qDeleteAll(_collections);
        _collections.clear();
    }
}
