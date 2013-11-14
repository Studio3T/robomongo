#include "robomongo/core/domain/MongoDatabase.h"

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/AppRegistry.h"
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
        EventsInfo::LoadCollectionInfo inf(_name);
        emit startedCollectionListLoad(inf);   
        _server->postEventToDataBase(new Events::LoadCollectionEvent(this, inf));
    }

    void MongoDatabase::loadUsers()
    {
        EventsInfo::LoadUserInfo inf(_name);
        emit startedUserListLoad(inf);
        _server->postEventToDataBase(new Events::LoadUserEvent(this, inf));
    }

    void MongoDatabase::loadFunctions()
    {
        EventsInfo::LoadFunctionInfo inf(_name);
        emit startedFunctionListLoad(inf);
        _server->postEventToDataBase(new Events::LoadFunctionEvent(this, inf));
    }

    void MongoDatabase::createCollection(const std::string &collection)
    {
        EventsInfo::CreateCollectionInfo inf(MongoNamespace(_name, collection));
        _server->postEventToDataBase(new Events::CreateCollectionEvent(this, inf));
    }

    void MongoDatabase::dropCollection(const std::string &collection)
    {
        EventsInfo::DropCollectionInfo inf(MongoNamespace(_name, collection));
        _server->postEventToDataBase(new Events::DropCollectionEvent(this, inf));
    }

    void MongoDatabase::renameCollection(const std::string &collection, const std::string &newCollection)
    {
        EventsInfo::RenameCollectionInfo inf(MongoNamespace(_name, collection), newCollection);
        _server->postEventToDataBase(new Events::RenameCollectionEvent(this, inf));
    }

    void MongoDatabase::duplicateCollection(const std::string &collection, const std::string &newCollection)
    {
        EventsInfo::DuplicateCollectionInfo inf(MongoNamespace(_name, collection), newCollection);
        _server->postEventToDataBase(new Events::DuplicateCollectionEvent(this, inf));
    }

    void MongoDatabase::copyCollection(MongoServer *server, const std::string &sourceDatabase, const std::string &collection)
    {
        EventsInfo::CopyCollectionToDiffServerInfo inf(server, MongoNamespace(sourceDatabase, collection), MongoNamespace(_name, collection));
        _server->postEventToDataBase(new Events::CopyCollectionToDiffServerEvent(this, inf));  
    }

    void MongoDatabase::createUser(const MongoUser &user, bool overwrite)
    {
        EventsInfo::CreateUserInfo inf(_name, user, overwrite);
        _server->postEventToDataBase(new Events::CreateUserEvent(this, inf));
    }

    void MongoDatabase::dropUser(const mongo::OID &id)
    {
        EventsInfo::DropUserInfo inf(_name, id);
        _server->postEventToDataBase(new Events::DropUserEvent(this, inf));
    }

    void MongoDatabase::createFunction(const MongoFunction &fun)
    {       
        EventsInfo::CreateFunctionInfo inf(_name, fun);
        _server->postEventToDataBase(new Events::CreateFunctionEvent(this, inf));
    }

    void MongoDatabase::updateFunction(const std::string &name, const MongoFunction &fun)
    {
        EventsInfo::CreateFunctionInfo inf(_name, fun, name);
        _server->postEventToDataBase(new Events::CreateFunctionEvent(this, inf));
    }

    void MongoDatabase::dropFunction(const std::string &name)
    {
        EventsInfo::DropFunctionInfo inf(_name, name);
        _server->postEventToDataBase(new Events::DropFunctionEvent(this, inf));
    }

    void MongoDatabase::customEvent(QEvent *event)
    {
        QEvent::Type type = event->type();
        if(type==static_cast<QEvent::Type>(Events::DropFunctionEvent::EventType))
        {
            Events::DropFunctionEvent *ev = static_cast<Events::DropFunctionEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::CreateFunctionEvent::EventType))
        {
            Events::CreateFunctionEvent *ev = static_cast<Events::CreateFunctionEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::LoadFunctionEvent::EventType)){
            Events::LoadFunctionEvent *ev = static_cast<Events::LoadFunctionEvent*>(event);
            Events::LoadFunctionEvent::value_type inf = ev->value();
            emit finishedFunctionListLoad(inf);
        }
        else if(type==static_cast<QEvent::Type>(Events::CreateUserEvent::EventType))
        {
            Events::CreateUserEvent *ev = static_cast<Events::CreateUserEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::DropUserEvent::EventType))
        {
            Events::DropUserEvent *ev = static_cast<Events::DropUserEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::LoadUserEvent::EventType)){
            Events::LoadUserEvent *ev = static_cast<Events::LoadUserEvent*>(event);
            Events::LoadUserEvent::value_type inf = ev->value();
            emit finishedUserListLoad(inf);
        }
        else if(type==static_cast<QEvent::Type>(Events::CreateCollectionEvent::EventType))
        {
            Events::CreateCollectionEvent *ev = static_cast<Events::CreateCollectionEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::DropCollectionEvent::EventType))
        {
            Events::DropCollectionEvent *ev = static_cast<Events::DropCollectionEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::RenameCollectionEvent::EventType))
        {
            Events::RenameCollectionEvent *ev = static_cast<Events::RenameCollectionEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::LoadCollectionEvent::EventType)){
            Events::LoadCollectionEvent *ev = static_cast<Events::LoadCollectionEvent*>(event);            
            EventsInfo::LoadCollectionInfo inf = ev->value();
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
        else if(type==static_cast<QEvent::Type>(Events::DuplicateCollectionEvent::EventType))
        {
            Events::DuplicateCollectionEvent *ev = static_cast<Events::DuplicateCollectionEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(Events::CopyCollectionToDiffServerEvent::EventType))
        {
            Events::CopyCollectionToDiffServerEvent *ev = static_cast<Events::CopyCollectionToDiffServerEvent*>(event);
        }
    }

    void MongoDatabase::clearCollections()
    {
        qDeleteAll(_collections);
        _collections.clear();
    }
}
