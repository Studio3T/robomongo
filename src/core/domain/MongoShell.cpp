#include "MongoShell.h"
#include "MongoCollection.h"
#include "MongoDocument.h"
#include "AppRegistry.h"
#include "EventBus.h"
#include "engine/Result.h"
#include "mongodb/MongoClient.h"
#include "MongoServer.h"

using namespace Robomongo;

MongoShell::MongoShell(MongoServer *server) :
    QObject(),
    _server(server),
    _client(server->client()),
    _bus(AppRegistry::instance().bus())
{

}

MongoShell::~MongoShell()
{
}

void MongoShell::open(MongoCollection *collection)
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
    QList<MongoShellResult> list = MongoShellResult::fromResult(event->result.results);
    MongoShellExecResult result(list,
                                event->result.currentServerName, event->result.isCurrentServerValid,
                                event->result.currentDatabaseName, event->result.isCurrentDatabaseValid);

    _bus->publish(new ScriptExecutedEvent(this, result));

    /*
    QList<MongoDocumentPtr> list;
    foreach(mongo::BSONObj obj, event->results) {
        MongoDocumentPtr doc(new MongoDocument(obj));
        list.append(doc);
    }

    _bus->publish(this, new ScriptExecutedEvent(event->response, list));*/
}
