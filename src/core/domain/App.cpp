#include "App.h"
#include "MongoServer.h"
#include "MongoShell.h"
#include "domain/MongoCollection.h"
#include "EventBus.h"

using namespace Robomongo;

App::App(EventBus *bus) :
    QObject(),
    _bus(bus)
{
}

MongoServer *App::openServer(const ConnectionRecordPtr &connectionRecord, bool visible)
{
    MongoServer *server = new MongoServer(connectionRecord, visible);
    _servers.append(server);

    if (visible)
        _bus->publish(new ConnectingEvent(this, server));

    server->tryConnect();
    return server;
}

void App::closeServer(MongoServer *server)
{
    _servers.removeOne(server);
    delete server;
}

MongoShellPtr App::openShell(const MongoCollectionPtr &collection)
{
    MongoServer *server = openServer(collection->database()->server()->connectionRecord(), false);

/*    ConnectionRecordPtr connectionRecord = collection->database()->server()->connectionRecord();
    MongoServerPtr serverClone(new MongoServer(connectionRecord));
    serverClone->tryConnect();
    _servers.append(serverClone);*/

    MongoShellPtr shell(new MongoShell(server));
    _shells.append(shell);

    QString script = QString("db.%1.find()").arg(collection->name());

    _bus->publish(new OpeningShellEvent(this, shell, script));

    shell->open(script, collection->database()->name());
    return shell;
}

MongoShellPtr App::openShell(MongoServer *server, const QString &script, const QString &dbName)
{
    MongoServer *serverClone = openServer(server->connectionRecord(), false);

    MongoShellPtr shell(new MongoShell(serverClone));
    _shells.append(shell);

    _bus->publish(new OpeningShellEvent(this, shell, script));

    shell->open(script, dbName);
    return shell;

}

void App::closeShell(const MongoShellPtr &shell)
{
    _shells.removeOne(shell);

    MongoServer *server = shell->server();
    _servers.removeOne(server);
    delete server; // TODO: duplicated logic
}
