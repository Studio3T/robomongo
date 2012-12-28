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

        /**
         * @brief Creates and
         * @param connectionInfo:
         * @param visible: should this server be visible in UI (explorer) or not.
         * @param defaultDatabase:
         * @return
         */
        MongoServer *openServer(ConnectionRecord *connectionSettings, bool visible,
                                const QString &defaultDatabase = QString());

        /**
         * @brief Closes MongoServer connection and frees all resources, owned
         * by specified MongoServer. Finally, specified MongoServer will also be deleted.
         */
        void closeServer(MongoServer *server);

        /**
         * @brief Open new shell based on specified collection
         */
        MongoShell *openShell(MongoCollection *collection);

        MongoShell *openShell(MongoServer *server, const QString &script,
                              const QString &dbName = QString(), bool execute = true,
                              const QString &shellName = QString());

        MongoShell *openShell(MongoDatabase *database, const QString &script,
                              bool execute = true, const QString &shellName = QString());

        /**
         * @brief Closes MongoShell and frees all resources, owned by specified MongoShell.
         * Finally, specified MongoShell will also be deleted.
         */
        void closeShell(MongoShell *shell);

    private:

        /**
         * @brief List of MongoServers, owned by this App.
         */
        QList<MongoServer *> _servers;

        /**
         * @brief List of MongoShells, owned by this App.
         */
        QList<MongoShell *> _shells;

        /**
         * @brief EventBus
         */
        EventBus *_bus;
    };
}


#endif // APPLICATION_H
