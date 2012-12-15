#include "MongoShell.h"
#include "MongoCollection.h"
#include "MongoDocument.h"
#include "AppRegistry.h"
#include "Dispatcher.h"
#include "engine/Result.h"
#include "mongodb/MongoClient.h"

using namespace Robomongo;

MongoShell::MongoShell(const MongoServerPtr server) :
    QObject(),
    _server(server),
    _client(server->client()),
    _dispatcher(&AppRegistry::instance().dispatcher())
{

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

bool MongoShell::event(QEvent *event)
{
    R_HANDLE(event)
    R_EVENT(ExecuteQueryResponse)
    R_EVENT(ExecuteScriptResponse)
    else return QObject::event(event);
}

void MongoShell::handle(const ExecuteQueryResponse *event)
{
    QList<MongoDocumentPtr> list;
    foreach(mongo::BSONObj obj, event->documents) {
        MongoDocumentPtr doc(new MongoDocument(obj));
        list.append(doc);
    }

    _dispatcher->publish(this, new DocumentListLoadedEvent(_query, list));
}

void MongoShell::handle(const ExecuteScriptResponse *event)
{
    QList<MongoShellResult> list = MongoShellResult::fromResult(event->results);

    _dispatcher->publish(this, new ScriptExecutedEvent(list));

    /*
    QList<MongoDocumentPtr> list;
    foreach(mongo::BSONObj obj, event->results) {
        MongoDocumentPtr doc(new MongoDocument(obj));
        list.append(doc);
    }

    _dispatcher->publish(this, new ScriptExecutedEvent(event->response, list));*/
}
