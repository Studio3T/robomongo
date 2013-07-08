#include "robomongo/core/domain/App.h"

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/MongoShell.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/EventBus.h"

namespace Robomongo
{

    App::App(EventBus *bus) : QObject(),
        _bus(bus) { }

    App::~App()
    {
        qDeleteAll(_shells);
        qDeleteAll(_servers);
    }

    /**
     * @brief Creates and opens new server connection.
     * @param connection: ConnectionSettings, that will be owned by MongoServer.
     * @param visible: should this server be visible in UI (explorer) or not.
     */
    MongoServer *App::openServer(ConnectionSettings *connection,
                                 bool visible)
    {
        MongoServer *server = new MongoServer(connection, visible);
        _servers.append(server);

        if (visible)
            _bus->publish(new ConnectingEvent(this, server));

        server->tryConnect();
        return server;
    }

    /**
     * @brief Closes MongoServer connection and frees all resources, owned
     * by MongoServer. Finally, specified MongoServer will also be deleted.
     */
    void App::closeServer(MongoServer *server)
    {
        // Do nothing, if this server not owned by this App.
        if (!_servers.contains(server))
            return;

        _servers.removeOne(server);
        delete server;
    }

    MongoShell *App::openShell(MongoCollection *collection,const QString &filePathToSave)
    {
        ConnectionSettings *connection = collection->database()->server()->connectionRecord()->clone();
        connection->setDefaultDatabase(collection->database()->name());
        QString script = buildCollectionQuery(collection->name(), "find()");
        return openShell(connection, ScriptInfo(script, true, CursorPosition(), collection->database()->name(),filePathToSave));
    }

    MongoShell *App::openShell(MongoServer *server, const QString &script, const QString &dbName,
                               bool execute, const QString &shellName,
                               const CursorPosition &cursorPosition,const QString &filePathToSave)
    {
        ConnectionSettings *connection = server->connectionRecord()->clone();

        if (!dbName.isEmpty())
            connection->setDefaultDatabase(dbName);

        return openShell(connection, ScriptInfo(script, execute, cursorPosition, shellName,filePathToSave));
    }

    MongoShell *App::openShell(MongoDatabase *database, const QString &script,
                               bool execute, const QString &shellName,
                               const CursorPosition &cursorPosition,const QString &filePathToSave)
    {
        ConnectionSettings *connection = database->server()->connectionRecord()->clone();
        connection->setDefaultDatabase(database->name());
        return openShell(connection, ScriptInfo(script, execute, cursorPosition, shellName,filePathToSave));
    }

    MongoShell *App::openShell(ConnectionSettings *connection, const ScriptInfo &scriptInfo)
    {
        MongoServer *server = openServer(connection, false);
        MongoShell *shell = new MongoShell(server,scriptInfo);

        _shells.append(shell);
        _bus->publish(new OpeningShellEvent(this, shell));

        if (scriptInfo.execute())
            shell->open(scriptInfo.script());
        else
            shell->open("");

        return shell;
    }

    QString App::buildCollectionQuery(const QString collectionName, const QString postfix)
    {
        QChar firstChar = collectionName.at(0);
        QRegExp charExp("[^A-Za-z_0-9]"); // valid JS name

        QString pattern;
        if  (firstChar == QChar('_')
             || collectionName == "help"
             || collectionName == "stats"
             || collectionName == "version"
             || collectionName == "prototype") {
            // TODO: this list should be expanded to include
            // all functions of DB JavaScript object
            pattern = "db.getCollection('%1').%2";
        } else if (firstChar.isDigit() || collectionName.contains(charExp)) {
            pattern = "db[\'%1\'].%2";
        } else {
            pattern = "db.%1.%2";
        }

        return pattern.arg(collectionName).arg(postfix);
    }

    /**
     * @brief Closes MongoShell and frees all resources, owned by specified MongoShell.
     * Finally, specified MongoShell will also be deleted.
     */
    void App::closeShell(MongoShell *shell)
    {
        // Do nothing, if this shell not owned by this App.
        if (!_shells.contains(shell))
            return;

        _shells.removeOne(shell);
        closeServer(shell->server());
        delete shell;
    }
}
