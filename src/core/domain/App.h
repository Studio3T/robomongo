#ifndef APPLICATION_H
#define APPLICATION_H

#include "Core.h"

namespace Robomongo
{
    class Dispatcher;

    class App : public QObject
    {
        Q_OBJECT
    public:
        App(Dispatcher *dispatcher);

        MongoServerPtr openServer(const ConnectionRecordPtr &connectionRecord);
    private:
        QList<MongoServerPtr> _servers;
        QList<MongoShellPtr> _shells;

        Dispatcher *_dispatcher;
    };
}


#endif // APPLICATION_H
