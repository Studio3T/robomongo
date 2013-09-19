#pragma once
#include <QObject>
#include <vector>

#include "robomongo/core/domain/ScriptInfo.h"

namespace Robomongo
{
    class EventBus;
    class MongoServer;
    class ConnectionSettings;
    class MongoCollection;
    class MongoShell;
    class MongoDatabase;
    namespace detail
    {
                /**
         * @brief Builds single collection query (i.e. db.my_col.find()) from
         *  string that doesn't contain "db.my_col." prefix.
         *
         *  If you'll call buildCollectionQuery("test", "find()") you'll receive:
         *  db.test.find()
         *
         *  If you'll call buildCollectionQuery("1234", "find()") you'll receive:
         *  db['1234'].find()
         *
         * @param script: query part (without "db.my_col." prefix"
         */
        QString buildCollectionQuery(const std::string &collectionName, const QString &postfix);
    }

    class App : public QObject
    {
        Q_OBJECT

    public:
        typedef std::vector<MongoServer*> MongoServersContainerType;
        typedef std::vector<MongoShell*> MongoShellsContainerType;
        App(EventBus *const bus);
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
        MongoShell *openShell(MongoCollection *collection,const QString &filePathToSave=QString());

        MongoShell *openShell(MongoServer *server, const QString &script, const std::string &dbName = std::string(),
                              bool execute = true, const QString &shellName = QString(),
                              const CursorPosition &cursorPosition = CursorPosition(),const QString &file=QString());

        MongoShell *openShell(MongoDatabase *database, const QString &script,
                              bool execute = true, const QString &shellName = QString(),
                              const CursorPosition &cursorPosition = CursorPosition(),const QString &filePathToSave=QString());

        MongoShell *openShell(ConnectionSettings *connection, const ScriptInfo &scriptInfo);
        MongoServersContainerType getServers() const {return _servers; };

        /**
         * @brief Closes MongoShell and frees all resources, owned by specified MongoShell.
         * Finally, specified MongoShell will also be deleted.
         */
        void closeShell(MongoShell *shell);

    private:
        /**
         * @brief MongoServers, owned by this App.
         */
        MongoServersContainerType _servers;

        /**
         * @brief MongoShells, owned by this App.
         */
        MongoShellsContainerType _shells;

        /**
         * @brief EventBus
         */
        EventBus *const _bus;
    };
}
