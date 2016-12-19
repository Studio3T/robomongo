#include "robomongo/core/domain/MongoDatabase.h"

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/mongodb/MongoWorker.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/utils/Logger.h"

namespace Robomongo
{
    R_REGISTER_EVENT(MongoDatabaseCollectionListLoadedEvent)
    R_REGISTER_EVENT(MongoDatabaseUsersLoadedEvent)
    R_REGISTER_EVENT(MongoDatabaseFunctionsLoadedEvent)
    R_REGISTER_EVENT(MongoDatabaseUsersLoadingEvent)
    R_REGISTER_EVENT(MongoDatabaseFunctionsLoadingEvent)
    R_REGISTER_EVENT(MongoDatabaseCollectionsLoadingEvent)

    const std::string MongoDatabase::StorageEngineType::WIRED_TIGER = "wiredTiger";
    const std::string MongoDatabase::StorageEngineType::MMAPV1      = "mmapv1";
 
    const float MongoDatabase::DBVersion::MONGODB_2_6 = 2.6f;
    const float MongoDatabase::DBVersion::MONGODB_3_0 = 3.0f;
    const float MongoDatabase::DBVersion::MONGODB_3_2 = 3.2f;

    MongoDatabase::MongoDatabase(MongoServer *server, const std::string &name) :
        QObject(),
        _system(name == "admin" || name == "local"),
        _server(server),
        _bus(AppRegistry::instance().bus()),
        _name(name) {}

    MongoDatabase::~MongoDatabase()
    {
        clearCollections();
    }

    void MongoDatabase::loadCollections()
    {
        _bus->publish(new MongoDatabaseCollectionsLoadingEvent(this));
        _bus->send(_server->worker(), new LoadCollectionNamesRequest(this, _name));
    }

    void MongoDatabase::loadUsers()
    {
        _bus->publish(new MongoDatabaseUsersLoadingEvent(this));
        _bus->send(_server->worker(), new LoadUsersRequest(this, _name));
    }

    void MongoDatabase::loadFunctions()
    {
        _bus->publish(new MongoDatabaseFunctionsLoadingEvent(this));
        _bus->send(_server->worker(), new LoadFunctionsRequest(this, _name));
    }

    void MongoDatabase::createCollection(const std::string &collection, long long size, bool capped, int maxDocNum, const mongo::BSONObj& extraOptions)
    {
        _bus->send(_server->worker(), 
            new CreateCollectionRequest(this, MongoNamespace(_name, collection), extraOptions, size, capped, maxDocNum));
    }

    void MongoDatabase::dropCollection(const std::string &collection)
    {
        _bus->send(_server->worker(), new DropCollectionRequest(this, MongoNamespace(_name, collection)));
    }

    void MongoDatabase::renameCollection(const std::string &collection, const std::string &newCollection)
    {
        _bus->send(_server->worker(), new RenameCollectionRequest(this, MongoNamespace(_name, collection), 
                                                                  newCollection));
    }

    void MongoDatabase::duplicateCollection(const std::string &collection, const std::string &newCollection)
    {
        _bus->send(_server->worker(), new DuplicateCollectionRequest(this, MongoNamespace(_name, collection), newCollection));
    }

    void MongoDatabase::copyCollection(MongoServer *server, const std::string &sourceDatabase, const std::string &collection)
    {
        _bus->send(_server->worker(), new CopyCollectionToDiffServerRequest(this, server->worker(), sourceDatabase, collection, _name));
    }

    void MongoDatabase::createUser(const MongoUser &user, bool overwrite)
    {
        _bus->send(_server->worker(), new CreateUserRequest(this, _name, user, overwrite));
    }

    void MongoDatabase::dropUser(const mongo::OID &id, std::string const& userName)
    {
        _bus->send(_server->worker(), new DropUserRequest(this, _name, id, userName));
    }

    void MongoDatabase::createFunction(const MongoFunction &fun)
    {
        _bus->send(_server->worker(), new CreateFunctionRequest(this, _name, fun));
    }

    void MongoDatabase::updateFunction(const std::string &name, const MongoFunction &fun)
    {
        _bus->send(_server->worker(), new CreateFunctionRequest(this, _name, fun, name));
    }

    void MongoDatabase::dropFunction(const std::string &name)
    {
        _bus->send(_server->worker(), new DropFunctionRequest(this, _name, name));
    }

    void MongoDatabase::handle(LoadCollectionNamesResponse *event)
    {
        if (event->isError()) {
            if (_server->connectionRecord()->isReplicaSet()) {  // replica set
                if (EventError::SetPrimaryUnreachable == event->error().errorCode())
                    _server->handle(&ReplicaSetRefreshed(this, event->error(), event->error().replicaSetInfo()));
            }
            else { // single server
                _bus->publish(new MongoDatabaseCollectionListLoadedEvent(this, event->error()));
            }

            genericResponseHandler(event, "Failed to refresh 'Collections'.");
            return;
        }

        clearCollections();

        for (auto const& collectionInfo : event->collectionInfos())
            addCollection(new MongoCollection(this, collectionInfo));

        _bus->publish(new MongoDatabaseCollectionListLoadedEvent(this, _collections));
        LOG_MSG("'Collections' refreshed.", mongo::logger::LogSeverity::Info());
    }

    void MongoDatabase::handle(CreateFunctionResponse *event)
    {
        if (event->isError()) {
            if (_server->connectionRecord()->isReplicaSet() &&
                EventError::SetPrimaryUnreachable == event->error().errorCode())
                _server->handle(&ReplicaSetRefreshed(this, event->error(), event->error().replicaSetInfo()));

            genericResponseHandler(event, "Failed to create function \'" + event->functionName + "\'.");
        }
        else {
            loadFunctions();
            LOG_MSG("Function \'" + event->functionName + "\' created.", mongo::logger::LogSeverity::Info());
        }
    }

    void MongoDatabase::handle(CreateUserResponse *event)
    {
        if (event->isError()) {
            if (_server->connectionRecord()->isReplicaSet() &&
                EventError::SetPrimaryUnreachable == event->error().errorCode())
                _server->handle(&ReplicaSetRefreshed(this, event->error(), event->error().replicaSetInfo()));

            genericResponseHandler(event, "Failed to create user \'" + event->userName + "\'.");
        }
        else {
            loadUsers();
            LOG_MSG("User \'" + event->userName + "\' created.", mongo::logger::LogSeverity::Info());
        }
    }

    void MongoDatabase::handle(LoadUsersResponse *event)
    {
        if (event->isError()) {
            _bus->publish(new MongoDatabaseUsersLoadedEvent(this, event->error()));
            return;
        }

        _bus->publish(new MongoDatabaseUsersLoadedEvent(this, this, event->users()));
    }

    void MongoDatabase::handle(LoadFunctionsResponse *event)
    {
        if (event->isError()) {
            if (_server->connectionRecord()->isReplicaSet()) {  // replica set
                if (EventError::SetPrimaryUnreachable == event->error().errorCode())
                    _server->handle(&ReplicaSetRefreshed(this, event->error(), event->error().replicaSetInfo()));
            }
            else { // single server
                _bus->publish(new MongoDatabaseFunctionsLoadedEvent(this, event->error()));
            }

            genericResponseHandler(event, "Failed to refresh 'Functions'.");
            return;
        }

        _bus->publish(new MongoDatabaseFunctionsLoadedEvent(this, this, event->functions()));
        LOG_MSG("'Functions' refreshed.", mongo::logger::LogSeverity::Info());
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

    void MongoDatabase::handle(CreateCollectionResponse *event) 
    {
        if (event->isError()) {
            if (_server->connectionRecord()->isReplicaSet() &&
                EventError::SetPrimaryUnreachable == event->error().errorCode())
                _server->handle(&ReplicaSetRefreshed(this, event->error(), event->error().replicaSetInfo()));

            genericResponseHandler(event, "Failed to create collection \'" + event->collection + "\'.");
        }
        else {
            loadCollections();
            LOG_MSG("Collection \'" + event->collection + "\' created.", mongo::logger::LogSeverity::Info());
        }
    }

    void MongoDatabase::handle(DropCollectionResponse *event) 
    {
        if (event->isError()) {
            if (_server->connectionRecord()->isReplicaSet() &&
                EventError::SetPrimaryUnreachable == event->error().errorCode())
                _server->handle(&ReplicaSetRefreshed(this, event->error(), event->error().replicaSetInfo()));

            genericResponseHandler(event, "Failed to drop collection \'" + event->collection + "\'.");
        }
        else {
            loadCollections();
            LOG_MSG("Collection \'" + event->collection + "\' dropped", mongo::logger::LogSeverity::Info());
        }
    }

    
    void MongoDatabase::handle(DropFunctionResponse *event)
    {
        if (event->isError()) {
            if (_server->connectionRecord()->isReplicaSet() &&
                EventError::SetPrimaryUnreachable == event->error().errorCode())
                _server->handle(&ReplicaSetRefreshed(this, event->error(), event->error().replicaSetInfo()));

            genericResponseHandler(event, "Failed to remove function \'" + event->functionName + "\'.");
        }
        else {
            loadFunctions();
            LOG_MSG("Function \'" + event->functionName + "\' removed", mongo::logger::LogSeverity::Info());
        }
    }

    void MongoDatabase::handle(DropUserResponse *event)
    {
        if (event->isError()) {
            if (_server->connectionRecord()->isReplicaSet() &&
                EventError::SetPrimaryUnreachable == event->error().errorCode())
                _server->handle(&ReplicaSetRefreshed(this, event->error(), event->error().replicaSetInfo()));

            genericResponseHandler(event, "Failed to drop user \'" + event->username + "\'.");
        }
        else {
            loadUsers();
            LOG_MSG("User \'" + event->username + "\' dropped", mongo::logger::LogSeverity::Info());
        }
    }

    void MongoDatabase::handle(RenameCollectionResponse *event)
    {
        if (event->isError()) {
            if (_server->connectionRecord()->isReplicaSet() &&
                EventError::SetPrimaryUnreachable == event->error().errorCode())
                _server->handle(&ReplicaSetRefreshed(this, event->error(), event->error().replicaSetInfo()));

            genericResponseHandler(event, "Failed to rename collection.");
        }
        else {
            loadCollections();
            LOG_MSG("Collection \'" + event->oldCollection + "\' renamed to \'" + 
                    event->newCollection +"\'." , mongo::logger::LogSeverity::Info());
        }
    }

    void MongoDatabase::handle(DuplicateCollectionResponse *event)
    {
        if (event->isError()) {
            if (_server->connectionRecord()->isReplicaSet() &&
                EventError::SetPrimaryUnreachable == event->error().errorCode())
                _server->handle(&ReplicaSetRefreshed(this, event->error(), event->error().replicaSetInfo()));

            genericResponseHandler(event, "Failed to duplicate collection \'" + event->sourceCollection + "\'.");
        }
        else {
            loadCollections();
            LOG_MSG("Collection \'" + event->sourceCollection + "\' duplicated as \'" +
                    event->duplicateCollection + "\'.", mongo::logger::LogSeverity::Info());
        }
    }

    void MongoDatabase::genericResponseHandler(Event *event, const std::string &userFriendlyMessage) {
        if (!event->isError())
            return;

        // Capitalize first char. (Mongo errors often come all lower case)
        std::string errMsg = event->error().errorMessage();
        if (!errMsg.empty())
            errMsg[0] = static_cast<char>(toupper(errMsg[0]));

        LOG_MSG(userFriendlyMessage + " " + errMsg, mongo::logger::LogSeverity::Error());
        _bus->publish(new OperationFailedEvent(this, errMsg, userFriendlyMessage));
    }
}
