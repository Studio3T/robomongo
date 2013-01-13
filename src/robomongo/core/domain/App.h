#pragma once

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/CursorPosition.h"
#include "robomongo/core/domain/ScriptInfo.h"

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
         * @brief Creates and opens new server connection.
         * @param connection: ConnectionSettings, that will be owned by MongoServer.
         * @param visible: should this server be visible in UI (explorer) or not.
         */
        MongoServer *openServer(ConnectionSettings *connection, bool visible);

        /**
         * @brief Closes MongoServer connection and frees all resources, owned
         * by specified MongoServer. Finally, specified MongoServer will also be deleted.
         */
        void closeServer(MongoServer *server);

        /**
         * @brief Open new shell based on specified collection
         */
        MongoShell *openShell(MongoCollection *collection);

        MongoShell *openShell(MongoServer *server, const QString &script, const QString &dbName = QString(),
                              bool execute = true, const QString &shellName = QString(),
                              const CursorPosition &cursorPosition = CursorPosition());

        MongoShell *openShell(MongoDatabase *database, const QString &script,
                              bool execute = true, const QString &shellName = QString(),
                              const CursorPosition &cursorPosition = CursorPosition());

        MongoShell *openShell(ConnectionSettings *connection, const ScriptInfo &scriptInfo);

        /**
         * @brief Closes MongoShell and frees all resources, owned by specified MongoShell.
         * Finally, specified MongoShell will also be deleted.
         */
        void closeShell(MongoShell *shell);

    private:
        /**
         * @brief MongoServers, owned by this App.
         */
        QList<MongoServer *> _servers;

        /**
         * @brief MongoShells, owned by this App.
         */
        QList<MongoShell *> _shells;

        /**
         * @brief EventBus
         */
        EventBus *_bus;
    };
}
