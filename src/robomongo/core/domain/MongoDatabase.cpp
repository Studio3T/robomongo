#include "robomongo/core/domain/MongoDatabase.h"

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/mongodb/MongoWorker.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"

namespace Robomongo
{

    R_REGISTER_EVENT(MongoDatabaseCollectionListLoadedEvent)
    R_REGISTER_EVENT(MongoDatabaseUsersLoadedEvent)
    R_REGISTER_EVENT(MongoDatabaseFunctionsLoadedEvent)
    R_REGISTER_EVENT(MongoDatabaseUsersLoadingEvent)
    R_REGISTER_EVENT(MongoDatabaseFunctionsLoadingEvent)
    R_REGISTER_EVENT(MongoDatabaseCollectionsLoadingEvent)

    MongoDatabase::MongoDatabase(MongoServer *server, const std::string &name) :
        QObject(),
        _system(name == "admin" || name == "local"),
        _server(server),
        _bus(AppRegistry::instance().bus()),
        _name(name)
    {
    }

    MongoDatabase::~MongoDatabase()
    {
        clearCollections();
    }

    void MongoDatabase::loadCollections()
    {
        _bus->publish(new MongoDatabaseCollectionsLoadingEvent(this));
        _bus->send(_server->client(), new LoadCollectionNamesRequest(this, _name));
    }

    void MongoDatabase::loadUsers()
    {
        _bus->publish(new MongoDatabaseUsersLoadingEvent(this));
        _bus->send(_server->client(), new LoadUsersRequest(this, _name));
    }

    void MongoDatabase::loadFunctions()
    {
        _bus->publish(new MongoDatabaseFunctionsLoadingEvent(this));
        _bus->send(_server->client(), new LoadFunctionsRequest(this, _name));
    }
    void MongoDatabase::createCollection(const std::string &collection)
    {
        _bus->send(_server->client(), new CreateCollectionRequest(this, MongoNamespace(_name, collection)));
    }

    void MongoDatabase::dropCollection(const std::string &collection)
    {
        _bus->send(_server->client(), new DropCollectionRequest(this, MongoNamespace(_name, collection)));
    }

    void MongoDatabase::renameCollection(const std::string &collection, const std::string &newCollection)
    {
        _bus->send(_server->client(), new RenameCollectionRequest(this, MongoNamespace(_name, collection), newCollection));
    }

    void MongoDatabase::duplicateCollection(const std::string &collection, const std::string &newCollection)
    {
        _bus->send(_server->client(), new DuplicateCollectionRequest(this, MongoNamespace(_name, collection), newCollection));
    }

    void MongoDatabase::copyCollection(MongoServer *server, const std::string &sourceDatabase, const std::string &collection)
    {
        _bus->send(_server->client(), new CopyCollectionToDiffServerRequest(this, server->client(), sourceDatabase, collection, _name));
    }

    void MongoDatabase::createUser(const MongoUser &user, bool overwrite)
    {
        _bus->send(_server->client(), new CreateUserRequest(this, _name, user, overwrite));
    }

    void MongoDatabase::dropUser(const mongo::OID &id)
    {
        _bus->send(_server->client(), new DropUserRequest(this, _name, id));
    }

    void MongoDatabase::createFunction(const MongoFunction &fun)
    {
        _bus->send(_server->client(), new CreateFunctionRequest(this, _name, fun));
    }

    void MongoDatabase::updateFunction(const std::string &name, const MongoFunction &fun)
    {
        _bus->send(_server->client(), new CreateFunctionRequest(this, _name, fun, name));
    }

    void MongoDatabase::dropFunction(const std::string &name)
    {
        _bus->send(_server->client(), new DropFunctionRequest(this, _name, name));
    }

    void MongoDatabase::handle(LoadCollectionNamesResponse *loaded)
    {
        if (loaded->isError())
            return;

        clearCollections();
        const std::vector<MongoCollectionInfo> &colectionsInfos = loaded->collectionInfos();
        for (std::vector<MongoCollectionInfo>::const_iterator it = colectionsInfos.begin(); it != colectionsInfos.end(); ++it) {
            const MongoCollectionInfo &info = *it;
            MongoCollection *collection = new MongoCollection(this, info);
            addCollection(collection);
        }

        _bus->publish(new MongoDatabaseCollectionListLoadedEvent(this, _collections));
    }

    void MongoDatabase::handle(CreateUserResponse *event)
    {
        if (event->isError())
            return;

    }

    void MongoDatabase::handle(LoadUsersResponse *event)
    {
        if (event->isError())
            return;
        
        _bus->publish(new MongoDatabaseUsersLoadedEvent(this, this, event->users()));
    }

    void MongoDatabase::handle(LoadFunctionsResponse *event)
    {
        if (event->isError())
            return;
            
        _bus->publish(new MongoDatabaseFunctionsLoadedEvent(this, this, event->functions()));
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
