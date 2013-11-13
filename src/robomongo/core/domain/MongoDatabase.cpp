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
        emit startedCollectionListLoad();
        LoadCollectionInfo inf(_name);
        _server->postEventToDataBase(new LoadCollectionEvent(this, inf));
    }

    void MongoDatabase::loadUsers()
    {
        emit startedUsersLoad();
        LoadUserInfo inf(_name);
        _server->postEventToDataBase(new LoadUserEvent(this, inf));
    }

    void MongoDatabase::loadFunctions()
    {
        emit startedFunctionsLoad();
        LoadFunctionInfo inf(_name);
        _server->postEventToDataBase(new LoadFunctionEvent(this, inf));
    }
    void MongoDatabase::createCollection(const std::string &collection)
    {
        CreateCollectionInfo inf(MongoNamespace(_name, collection));
        _server->postEventToDataBase(new CreateCollectionEvent(this, inf));
    }

    void MongoDatabase::dropCollection(const std::string &collection)
    {
        DropCollectionInfo inf(MongoNamespace(_name, collection));
        _server->postEventToDataBase(new DropCollectionEvent(this, inf));
    }

    void MongoDatabase::renameCollection(const std::string &collection, const std::string &newCollection)
    {
        RenameCollectionInfo inf(MongoNamespace(_name, collection), newCollection);
        _server->postEventToDataBase(new RenameCollectionEvent(this, inf));
    }

    void MongoDatabase::duplicateCollection(const std::string &collection, const std::string &newCollection)
    {
        DuplicateCollectionInfo inf(MongoNamespace(_name, collection), newCollection);
        _server->postEventToDataBase(new DuplicateCollectionEvent(this, inf));
    }

    void MongoDatabase::copyCollection(MongoServer *server, const std::string &sourceDatabase, const std::string &collection)
    {
        CopyCollectionToDiffServerInfo inf(server, MongoNamespace(sourceDatabase, collection), MongoNamespace(_name, collection));
        _server->postEventToDataBase(new CopyCollectionToDiffServerEvent(this, inf));  
    }

    void MongoDatabase::createUser(const MongoUser &user, bool overwrite)
    {
        CreateUserInfo inf(_name, user, overwrite);
        _server->postEventToDataBase(new CreateUserEvent(this, inf));
    }

    void MongoDatabase::dropUser(const mongo::OID &id)
    {
        DropUserInfo inf(_name, id);
        _server->postEventToDataBase(new DropUserEvent(this, inf));
    }

    void MongoDatabase::createFunction(const MongoFunction &fun)
    {       
        CreateFunctionInfo inf(_name, fun);
        _server->postEventToDataBase(new CreateFunctionEvent(this, inf));
    }

    void MongoDatabase::updateFunction(const std::string &name, const MongoFunction &fun)
    {
        CreateFunctionInfo inf(_name, fun, name);
        _server->postEventToDataBase(new CreateFunctionEvent(this, inf));
    }

    void MongoDatabase::dropFunction(const std::string &name)
    {
        DropFunctionInfo inf(_name, name);
        _server->postEventToDataBase(new DropFunctionEvent(this, inf));
    }

    void MongoDatabase::customEvent(QEvent *event)
    {
        QEvent::Type type = event->type();
        if(type==static_cast<QEvent::Type>(DropFunctionEvent::EventType))
        {
            DropFunctionEvent *ev = static_cast<DropFunctionEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(CreateFunctionEvent::EventType))
        {
            CreateFunctionEvent *ev = static_cast<CreateFunctionEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(LoadFunctionEvent::EventType)){
            LoadFunctionEvent *ev = static_cast<LoadFunctionEvent*>(event);
            ErrorInfo er = ev->errorInfo();
            if (!er.isError()){
                emit functionsListLoaded(ev->value()._functions);
            }
        }
        else if(type==static_cast<QEvent::Type>(CreateUserEvent::EventType))
        {
            CreateUserEvent *ev = static_cast<CreateUserEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(DropUserEvent::EventType))
        {
            DropUserEvent *ev = static_cast<DropUserEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(LoadUserEvent::EventType)){
            LoadUserEvent *ev = static_cast<LoadUserEvent*>(event);
            ErrorInfo er = ev->errorInfo();
            if (!er.isError()){
                emit userListLoaded(ev->value()._users);
            }
        }
        else if(type==static_cast<QEvent::Type>(CreateCollectionEvent::EventType))
        {
            CreateCollectionEvent *ev = static_cast<CreateCollectionEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(DropCollectionEvent::EventType))
        {
            DropCollectionEvent *ev = static_cast<DropCollectionEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(RenameCollectionEvent::EventType))
        {
            RenameCollectionEvent *ev = static_cast<RenameCollectionEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(LoadCollectionEvent::EventType)){
            LoadCollectionEvent *ev = static_cast<LoadCollectionEvent*>(event);
            ErrorInfo er = ev->errorInfo();
            LoadCollectionInfo inf = ev->value();
            if (!er.isError()){
                clearCollections();
                for (std::vector<MongoCollectionInfo>::const_iterator it = inf._infos.begin(); it != inf._infos.end(); ++it) {
                    const MongoCollectionInfo &info = *it;
                    MongoCollection *collection = new MongoCollection(this, info);
                    addCollection(collection);
                }
                emit collectionListLoaded(_collections);
            }            
        }
        else if(type==static_cast<QEvent::Type>(DuplicateCollectionEvent::EventType))
        {
            DuplicateCollectionEvent *ev = static_cast<DuplicateCollectionEvent*>(event);
        }
        else if(type==static_cast<QEvent::Type>(CopyCollectionToDiffServerEvent::EventType))
        {
            CopyCollectionToDiffServerEvent *ev = static_cast<CopyCollectionToDiffServerEvent*>(event);
        }
    }

    void MongoDatabase::clearCollections()
    {
        qDeleteAll(_collections);
        _collections.clear();
    }

    void MongoDatabase::addCollection(MongoCollection *collection)
    {
        _collections.push_back(collection);
    }
}
