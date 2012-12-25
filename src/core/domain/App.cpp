#include "App.h"
#include "MongoServer.h"
#include "MongoShell.h"
#include "domain/MongoCollection.h"
#include "EventBus.h"

using namespace Robomongo;

App::App(EventBus *bus) : QObject(),
    _bus(bus)
{
}

App::~App()
{
    qDeleteAll(_shells);
    qDeleteAll(_servers);
}

MongoServer *App::openServer(ConnectionRecord *connectionRecord, bool visible, MongoDatabase *defaultDatabase)
{
    MongoServer *server = new MongoServer(connectionRecord, visible, defaultDatabase);
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

MongoShell *App::openShell(MongoCollection *collection)
{
    MongoServer *server = openServer(collection->database()->server()->connectionRecord(), false);

/*    ConnectionRecordPtr connectionRecord = collection->database()->server()->connectionRecord();
    MongoServerPtr serverClone(new MongoServer(connectionRecord));
    serverClone->tryConnect();
    _servers.append(serverClone);*/

    MongoShell *shell = new MongoShell(server);
    _shells.append(shell);

    QString script = QString("db.%1.find()").arg(collection->name());

    _bus->publish(new OpeningShellEvent(this, shell, script));

    shell->open(script, collection->database()->name());
    return shell;
}

MongoShell *App::openShell(MongoServer *server, const QString &script, const QString &dbName, bool execute, const QString &shellName)
{
    MongoServer *serverClone = openServer(server->connectionRecord(), false);

    MongoShell *shell = new MongoShell(serverClone);
    _shells.append(shell);

    _bus->publish(new OpeningShellEvent(this, shell, script, shellName));

    if (execute)
        shell->open(script, dbName);

    return shell;

}

MongoShell *App::openShell(MongoDatabase *database, const QString &script, bool execute, const QString &shellName)
{
    MongoServer *serverClone = openServer(database->server()->connectionRecord(), false, database);

    MongoShell *shell = new MongoShell(serverClone);
    _shells.append(shell);

    _bus->publish(new OpeningShellEvent(this, shell, script, shellName));

    if (execute)
        shell->open(script, database->name());

    return shell;
}

void App::closeShell(MongoShell *shell)
{
    _shells.removeOne(shell);
    closeServer(shell->server());
    delete shell;
}
