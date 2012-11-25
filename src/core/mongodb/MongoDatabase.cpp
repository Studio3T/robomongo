#include "MongoDatabase.h"
#include "MongoServer.h"
#include "MongoCollection.h"
#include "events/MongoEvents.h"
#include "AppRegistry.h"
#include "Dispatcher.h"

using namespace Robomongo;

MongoDatabase::MongoDatabase(MongoServer *server, const QString &name) : QObject(),
    _system(false),
    _dispatcher(AppRegistry::instance().dispatcher())
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

/**
 * @brief Events dispatcher
 */
bool MongoDatabase::event(QEvent *event)
{
    R_HANDLE(event) {
        R_EVENT(LoadCollectionNamesResponse);
    }
}

void MongoDatabase::handle(const LoadCollectionNamesResponse *loaded)
{
    QList<MongoCollectionPtr> list;

    foreach(QString name, loaded->collectionNames)    {
        MongoCollectionPtr db(new MongoCollection(this, name));
        list.append(db);
    }

    _dispatcher.publish(this, new CollectionListLoadedEvent(list));
}
