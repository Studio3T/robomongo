#pragma once
#include <QObject>
#include <vector>
#include <robomongo/core/events/MongoEvents.h>

#include "robomongo/core/domain/ScriptInfo.h"

namespace Robomongo
{
    class EventBus;
    class MongoServer;
    class ConnectionSettings;
    class MongoCollection;
    class MongoShell;
    class MongoDatabase;
    class EstablishSshConnectionResponse;
    class LogEvent;

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
        MongoServer *openServer(ConnectionSettings *connection, ConnectionType type);

        /**
         * @brief Closes MongoServer connection and frees all resources, owned
         * by specified MongoServer. Finally, specified MongoServer will also be deleted.
         */
        void closeServer(MongoServer *server);

        /**
         * @brief Open new shell based on specified collection
         */
        MongoShell *openShell(MongoCollection *collection, const QString &filePathToSave = QString());

        MongoShell *openShell(MongoServer *server, const QString &script, const std::string &dbName = std::string(),
                              bool execute = true, const QString &shellName = QString(),
                              const CursorPosition &cursorPosition = CursorPosition(), const QString &file = QString());

        MongoShell *openShell(MongoDatabase *database, const QString &script,
                              bool execute = true, const QString &shellName = QString(),
                              const CursorPosition &cursorPosition = CursorPosition(), const QString &filePathToSave = QString());

        MongoShell *openShell(ConnectionSettings *connection, const ScriptInfo &scriptInfo);
        MongoServersContainerType getServers() const { return _servers; };

        /**
         * @brief Closes MongoShell and frees all resources, owned by specified MongoShell.
         * Finally, specified MongoShell will also be deleted.
         */
        void closeShell(MongoShell *shell);

        void fireConnectionFailedEvent(int serverHandle, ConnectionType type, std::string errormsg, ConnectionFailedEvent::Reason reason);

        int getLastServerHandle() const { return _lastServerHandle; };

    public Q_SLOTS:
        void handle(EstablishSshConnectionResponse *event);
        void handle(ListenSshConnectionResponse *event);
        void handle(LogEvent *event);

    private:
        MongoServer *continueOpenServer(int serverHandle, ConnectionSettings *connection, ConnectionType type, int localport = 0);

        /**
         * MongoServers, owned by this App.
         */
        MongoServersContainerType _servers;

        /**
         * MongoShells, owned by this App.
         */
        MongoShellsContainerType _shells;

        EventBus *const _bus;

        // Increase monotonically when new MongoServer is created
        // Never decreases.
        int _lastServerHandle;
    };
}
