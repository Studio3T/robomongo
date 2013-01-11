#include "robomongo/core/domain/MongoDatabase.h"

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"

using namespace Robomongo;

R_REGISTER_EVENT(MongoDatabase_CollectionListLoadedEvent)

MongoDatabase::MongoDatabase(MongoServer *server, const QString &name) : QObject(),
    _system(false),
    _server(server),
    _bus(AppRegistry::instance().bus())
{
    _name = name;
    _client = _server->client();

    // Check that this is a system database
    _system = name == "admin" ||
              name == "local";
}

MongoDatabase::~MongoDatabase()
{
    clearCollections();
}

void MongoDatabase::loadCollections()
{
    _bus->send(_client, new LoadCollectionNamesRequest(this, _name));
}

void MongoDatabase::handle(LoadCollectionNamesResponse *loaded)
{
    clearCollections();

    foreach(CollectionInfo name, loaded->collectionNames)    {
        MongoCollection *collection = new MongoCollection(this, name);
        addCollection(collection);
    }

    _bus->publish(new MongoDatabase_CollectionListLoadedEvent(this, _collections));
}

void MongoDatabase::clearCollections()
{
    qDeleteAll(_collections);
    _collections.clear();
}

void MongoDatabase::addCollection(MongoCollection *collection)
{
    _collections.append(collection);
}
