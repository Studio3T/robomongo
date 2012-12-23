#ifndef APPLICATION_H
#define APPLICATION_H

#include "Core.h"

namespace Robomongo
{
    class EventBus;

    class App : public QObject
    {
        Q_OBJECT
    public:
        App(EventBus *bus);
        ~App();

        MongoServer *openServer(ConnectionRecord *connectionRecord, bool visible);
        void closeServer(MongoServer *server);

        /**
         * @brief Open new shell based on specified collection
         */
        MongoShellPtr openShell(const MongoCollectionPtr &collection);
        MongoShellPtr openShell(MongoServer *server, const QString &script, const QString &dbName = QString());
        void closeShell(const MongoShellPtr &shell);

    private:

        QList<MongoServer *> _servers;
        QList<MongoShellPtr> _shells;

        EventBus *_bus;
    };
}


#endif // APPLICATION_H
