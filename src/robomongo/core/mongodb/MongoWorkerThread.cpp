#include "robomongo/core/mongodb/MongoWorkerThread.h"
#include "robomongo/core/mongodb/MongoWorker.h"

namespace Robomongo
{
    MongoWorkerThread::MongoWorkerThread(MongoWorker *client) : QThread(),
        _client(client)
    {
    }

    void MongoWorkerThread::run()
    {
        exec();
        if (_client->engine())
            delete _client->engine();
    }
}
