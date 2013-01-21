#include "robomongo/core/domain/MongoShell.h"

#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/mongodb/MongoWorker.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"

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
//    _query = QString("db.%1.find()").arg(collection->name());
//    _client->send(new ExecuteQueryRequest(this, collection->fullName()));
}

void MongoShell::open(const QString &script, const QString &dbName)
{
    _query = script;
    _client->send(new ExecuteScriptRequest(this, _query, dbName));
}

void MongoShell::query(int resultIndex, const MongoQueryInfo &info)
{
    _client->send(new ExecuteQueryRequest(this, resultIndex, info));
}

void MongoShell::handle(ExecuteQueryResponse *event)
{
    _bus->publish(new DocumentListLoadedEvent(this, event->resultIndex, event->queryInfo, _query, event->documents));
}

void MongoShell::handle(ExecuteScriptResponse *event)
{
    _bus->publish(new ScriptExecutedEvent(this, event->result, event->empty));
}
