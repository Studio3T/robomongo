#include "robomongo/core/domain/App.h"

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/MongoShell.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/utils/StdUtils.h"
#include "robomongo/core/utils/Logger.h"

namespace Robomongo
{

    App::App(EventBus *const bus) : QObject(),
        _bus(bus) { }

    App::~App()
    {
        std::for_each(_shells.begin(), _shells.end(), stdutils::default_delete<MongoShell*>());
        std::for_each(_servers.begin(), _servers.end(), stdutils::default_delete<MongoServer*>());
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
        _servers.push_back(server);

        if (visible)
            _bus->publish(new ConnectingEvent(this, server));

        LOG_MSG(QString("Connecting to %1...").arg(QtUtils::toQString(server->connectionRecord()->getFullAddress())));
        server->tryConnect();
        return server;
    }

    /**
     * @brief Closes MongoServer connection and frees all resources, owned
     * by MongoServer. Finally, specified MongoServer will also be deleted.
     */
    void App::closeServer(MongoServer *server)
    {
        _servers.erase(std::remove_if(_servers.begin(), _servers.end(), stdutils::RemoveIfFound<MongoServer*>(server)), _servers.end());
    }

    MongoShell *App::openShell(MongoCollection *collection,const QString &filePathToSave)
    {
        ConnectionSettings *connection = collection->database()->server()->connectionRecord()->clone();
        connection->setDefaultDatabase(collection->database()->name());
        QString script = buildCollectionQuery(collection->name(), "find()");
        return openShell(connection, ScriptInfo(script, true, CursorPosition(), QtUtils::toQString(collection->database()->name()),filePathToSave));
    }

    MongoShell *App::openShell(MongoServer *server,const QString &script, const std::string &dbName,
                               bool execute, const QString &shellName,
                               const CursorPosition &cursorPosition,const QString &filePathToSave)
    {
        ConnectionSettings *connection = server->connectionRecord()->clone();

        if (!dbName.empty())
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
        _shells.push_back(shell);
        _bus->publish(new OpeningShellEvent(this, shell));
        LOG_MSG("Openning shell...");
        shell->execute();
        return shell;
    }

    QString App::buildCollectionQuery(const std::string &collectionName, const QString &postfix)
    {
        QString qCollectionName = QtUtils::toQString(collectionName);
        QChar firstChar = qCollectionName.at(0);
        QRegExp charExp("[^A-Za-z_0-9]"); // valid JS name

        QString pattern;
        if  (firstChar == QChar('_')
             || qCollectionName == "help"
             || qCollectionName == "stats"
             || qCollectionName == "version"
             || qCollectionName == "prototype") {
            // TODO: this list should be expanded to include
            // all functions of DB JavaScript object
            pattern = "db.getCollection('%1').%2";
        } else if (firstChar.isDigit() || qCollectionName.contains(charExp)) {
            pattern = "db[\'%1\'].%2";
        } else {
            pattern = "db.%1.%2";
        }

        return pattern.arg(qCollectionName).arg(postfix);
    }

    /**
     * @brief Closes MongoShell and frees all resources, owned by specified MongoShell.
     * Finally, specified MongoShell will also be deleted.
     */
    void App::closeShell(MongoShell *shell)
    {
        // Do nothing, if this shell not owned by this App.
        MongoShellsContainerType::iterator it = std::find_if(_shells.begin(),_shells.end(),std::bind1st(std::equal_to<MongoShell *>(),shell));
        if (it == _shells.end())
            return;

        _shells.erase(it);
        closeServer(shell->server());
        delete shell;
    }
}
