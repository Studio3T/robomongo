#include "MongoShell.h"
#include "MongoCollection.h"
#include "MongoDocument.h"
#include "AppRegistry.h"
#include "EventBus.h"
#include "engine/Result.h"
#include "mongodb/MongoClient.h"

using namespace Robomongo;

MongoShell::MongoShell(const MongoServerPtr server) :
    QObject(),
    _server(server),
    _client(server->client()),
    _bus(&AppRegistry::instance().bus())
{

}

MongoShell::~MongoShell()
{
    int t = 56;
}

void MongoShell::open(const MongoCollectionPtr &collection)
{
    _query = QString("db.%1.find()").arg(collection->name());
    _client->send(new ExecuteQueryRequest(this, collection->fullName()));
}

void MongoShell::open(const QString &script, const QString &dbName)
{
    _query = script;
    _client->send(new ExecuteScriptRequest(this, _query, dbName));
}

void MongoShell::handle(ExecuteQueryResponse *event)
{
    QList<MongoDocumentPtr> list;
    foreach(mongo::BSONObj obj, event->documents) {
        MongoDocumentPtr doc(new MongoDocument(obj));
        list.append(doc);
    }

    _bus->publish(new DocumentListLoadedEvent(this, _query, list));
}

void MongoShell::handle(ExecuteScriptResponse *event)
{
    QList<MongoShellResult> list = MongoShellResult::fromResult(event->results);

    _bus->publish(new ScriptExecutedEvent(this, list));

    /*
    QList<MongoDocumentPtr> list;
    foreach(mongo::BSONObj obj, event->results) {
        MongoDocumentPtr doc(new MongoDocument(obj));
        list.append(doc);
    }

    _bus->publish(this, new ScriptExecutedEvent(event->response, list));*/
}
