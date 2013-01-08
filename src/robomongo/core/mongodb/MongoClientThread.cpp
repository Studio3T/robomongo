#include "MongoClientThread.h"
#include "MongoClient.h"

using namespace Robomongo;

MongoClientThread::MongoClientThread(MongoClient *client) : QThread(),
    _client(client)
{
}

void MongoClientThread::run()
{
    exec();

    if (_client->engine())
        delete _client->engine();
}
