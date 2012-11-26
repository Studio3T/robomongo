#include "App.h"
#include "MongoServer.h"
#include "Dispatcher.h"

using namespace Robomongo;

App::App(Dispatcher *dispatcher) :
    QObject(),
    _dispatcher(dispatcher)
{
}

MongoServerPtr App::openServer(const ConnectionRecordPtr &connectionRecord)
{
    MongoServerPtr server(new MongoServer(connectionRecord));
    _servers.append(server);

    _dispatcher->publish(this, new ConnectingEvent(server));
    server->tryConnect();
    return server;
}
