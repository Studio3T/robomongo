#include "MongoDatabase.h"
#include "MongoServer.h"
#include "MongoCollection.h"
#include "events/MongoEvents.h"

using namespace Robomongo;

MongoDatabase::MongoDatabase(MongoServer *server, const QString &name) : QObject(),
    _system(false)
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
    _client->loadCollectionNames(this, _name);
}

/**
 * @brief Events dispatcher
 */
bool MongoDatabase::event(QEvent *event)
{
    if (CollectionNamesLoaded::EventType == event->type())
        handle(static_cast<CollectionNamesLoaded *>(event));
}

void MongoDatabase::handle(const CollectionNamesLoaded *loaded)
{
    QList<MongoCollectionPtr> list;

    foreach(QString name, loaded->collectionNames())    {
        MongoCollectionPtr db(new MongoCollection(this, name));
        list.append(db);
    }

    emit collectionListLoaded(list);
}
