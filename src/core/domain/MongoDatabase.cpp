#include "MongoDatabase.h"
#include "MongoServer.h"
#include "MongoCollection.h"
#include "events/MongoEvents.h"
#include "AppRegistry.h"
#include "EventBus.h"

using namespace Robomongo;

R_REGISTER_EVENT(MongoDatabase_CollectionListLoadedEvent)

MongoDatabase::MongoDatabase(MongoServer *server, const QString &name) : QObject(),
    _system(false),
    _bus(AppRegistry::instance().bus())
{
    _server = server;
    _name = name;
    _client = _server->client();

    // Check that this is a system database
    _system = name == "admin" ||
              name == "local";
}

MongoDatabase::~MongoDatabase()
{

}

void MongoDatabase::listCollections()
{
    _client->send(new LoadCollectionNamesRequest(this, _name));
}

void MongoDatabase::handle(LoadCollectionNamesResponse *loaded)
{
    QList<MongoCollectionPtr> list;

    foreach(QString name, loaded->collectionNames)    {
        MongoCollectionPtr db(new MongoCollection(this, name));
        list.append(db);
    }

    _bus.publish(new MongoDatabase_CollectionListLoadedEvent(this, list));
}
