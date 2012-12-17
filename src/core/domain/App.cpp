#include "App.h"
#include "MongoServer.h"
#include "MongoShell.h"
#include "domain/MongoCollection.h"
#include "Dispatcher.h"

using namespace Robomongo;

App::App(Dispatcher *dispatcher) :
    QObject(),
    _dispatcher(dispatcher)
{
}

MongoServerPtr App::openServer(const ConnectionRecordPtr &connectionRecord, bool visible)
{
    MongoServerPtr server(new MongoServer(connectionRecord, visible));
    _servers.append(server);

    if (visible)
        _dispatcher->publish(this, new ConnectingEvent(server));

    server->tryConnect();
    return server;
}

MongoShellPtr App::openShell(const MongoCollectionPtr &collection)
{
    MongoServerPtr server(openServer(collection->database()->server()->connectionRecord(), false));

/*    ConnectionRecordPtr connectionRecord = collection->database()->server()->connectionRecord();
    MongoServerPtr serverClone(new MongoServer(connectionRecord));
    serverClone->tryConnect();
    _servers.append(serverClone);*/

    MongoShellPtr shell(new MongoShell(server));
    _shells.append(shell);

    QString script = QString("db.%1.find()").arg(collection->name());

    _dispatcher->publish(this, new OpeningShellEvent(shell, script));

    shell->open(script, collection->database()->name());
    return shell;
}

MongoShellPtr App::openShell(const MongoServerPtr &server, const QString &script, const QString &dbName)
{
    MongoServerPtr serverClone(openServer(server->connectionRecord(), false));

    MongoShellPtr shell(new MongoShell(serverClone));
    _shells.append(shell);

    _dispatcher->publish(this, new OpeningShellEvent(shell, script));

    shell->open(script, dbName);
    return shell;

}
