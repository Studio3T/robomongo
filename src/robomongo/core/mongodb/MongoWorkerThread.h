#pragma once
#include <QThread>
namespace Robomongo
{
    class MongoWorker;
    class MongoWorkerThread : public QThread
    {
        Q_OBJECT
    public:
        explicit MongoWorkerThread(MongoWorker *client);
        MongoWorker *client() const { return _client; }
    protected:
        void run();
    private:
        MongoWorker *_client;
    };
}
