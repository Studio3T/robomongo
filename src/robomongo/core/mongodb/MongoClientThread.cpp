#include "robomongo/core/mongodb/MongoClientThread.h"
#include "robomongo/core/mongodb/MongoClient.h"

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
