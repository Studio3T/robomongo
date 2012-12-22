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

        MongoServerPtr openServer(const ConnectionRecordPtr &connectionRecord, bool visible);
        void closeServer(const MongoServerPtr &server);

        /**
         * @brief Open new shell based on specified collection
         */
        MongoShellPtr openShell(const MongoCollectionPtr &collection);
        MongoShellPtr openShell(const MongoServerPtr &server, const QString &script, const QString &dbName = QString());
        void closeShell(const MongoShellPtr &shell);

    private:
        QList<MongoServerPtr> _servers;
        QList<MongoShellPtr> _shells;

        Dispatcher *_dispatcher;
    };
}


#endif // APPLICATION_H
