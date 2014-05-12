#include "robomongo/core/domain/App.h"
#include <QHash>

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
    namespace detail
    {
        QHash<QString, int> reservedNames()
        {
            // To get list of this names, execute the following
            // query in the mongo shell:
            // for (var field in db) { print(field) }
            //
            // We omited all fields that start with '_' symbol.
            // This case is covered by buildCollectionQuery() function.
            QHash<QString, int> names;
            names.insert("prototype", 0);
            names.insert("system.indexes", 0);
            names.insert("getMongo", 0);
            names.insert("getSiblingDB", 0);
            names.insert("getSisterDB", 0);
            names.insert("getName", 0);
            names.insert("stats", 0);
            names.insert("getCollection", 0);
            names.insert("commandHelp", 0);
            names.insert("runCommand", 0);
            names.insert("adminCommand", 0);
            names.insert("addUser", 0);
            names.insert("changeUserPassword", 0);
            names.insert("logout", 0);
            names.insert("removeUser", 0);
            names.insert("auth", 0);
            names.insert("createCollection", 0);
            names.insert("getProfilingLevel", 0);
            names.insert("getProfilingStatus", 0);
            names.insert("dropDatabase", 0);
            names.insert("shutdownServer", 0);
            names.insert("cloneDatabase", 0);
            names.insert("cloneCollection", 0);
            names.insert("copyDatabase", 0);
            names.insert("repairDatabase", 0);
            names.insert("help", 0);
            names.insert("printCollectionStats", 0);
            names.insert("setProfilingLevel", 0);
            names.insert("eval", 0);
            names.insert("dbEval", 0);
            names.insert("groupeval", 0);
            names.insert("groupcmd", 0);
            names.insert("group", 0);
            names.insert("resetError", 0);
            names.insert("forceError", 0);
            names.insert("getLastError", 0);
            names.insert("getLastErrorObj", 0);
            names.insert("getLastErrorCmd", 0);
            names.insert("getPrevError", 0);
            names.insert("getCollectionNames", 0);
            names.insert("tojson", 0);
            names.insert("toString", 0);
            names.insert("isMaster", 0);
            names.insert("currentOp", 0);
            names.insert("currentOP", 0);
            names.insert("killOp", 0);
            names.insert("killOP", 0);
            names.insert("getReplicationInfo", 0);
            names.insert("printReplicationInfo", 0);
            names.insert("printSlaveReplicationInfo", 0);
            names.insert("serverBuildInfo", 0);
            names.insert("serverStatus", 0);
            names.insert("hostInfo", 0);
            names.insert("serverCmdLineOpts", 0);
            names.insert("version", 0);
            names.insert("serverBits", 0);
            names.insert("listCommands", 0);
            names.insert("printShardingStatus", 0);
            names.insert("fsyncLock", 0);
            names.insert("fsyncUnlock", 0);
            names.insert("setSlaveOk", 0);
            names.insert("getSlaveOk", 0);
            names.insert("loadServerScripts", 0);
            return names;
        }

        QString buildCollectionQuery(const std::string &collectionName, const QString &postfix)
        {
            static QHash<QString, int> reserved = reservedNames();

            QString qCollectionName = QtUtils::toQString(collectionName);
            QChar firstChar = qCollectionName.at(0);

            // Regexp for invalid JS name, which
            // does not contain the following characters:
            QRegExp charExp("[^A-Za-z_0-9]");

            QString pattern;
            if (firstChar == QChar('_') || reserved.contains(qCollectionName)) {
                pattern = "db.getCollection('%1').%2";
            } else if (firstChar.isDigit() || qCollectionName.contains(charExp)) {
                pattern = "db[\'%1\'].%2";
            } else {
                pattern = "db.%1.%2";
            }

            // Escape '\' symbol
            qCollectionName.replace(QChar('\\'), "\\\\");

            return pattern.arg(qCollectionName).arg(postfix);
        }
    }

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

        LOG_MSG(QString("Connecting to %1...").arg(QtUtils::toQString(server->connectionRecord()->getFullAddress())), mongo::LL_INFO);
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
        ConnectionSettings *connection = collection->database()->server()->connectionRecord();
        connection->setDefaultDatabase(collection->database()->name());
        QString script = detail::buildCollectionQuery(collection->name(), "find({})");
        return openShell(connection, ScriptInfo(script, true, CursorPosition(), QtUtils::toQString(collection->database()->name()),filePathToSave));
    }

    MongoShell *App::openShell(MongoServer *server,const QString &script, const std::string &dbName,
                               bool execute, const QString &shellName,
                               const CursorPosition &cursorPosition,const QString &filePathToSave)
    {
        ConnectionSettings *connection = server->connectionRecord();

        if (!dbName.empty())
            connection->setDefaultDatabase(dbName);

        return openShell(connection, ScriptInfo(script, execute, cursorPosition, shellName,filePathToSave));
    }

    MongoShell *App::openShell(MongoDatabase *database, const QString &script,
                               bool execute, const QString &shellName,
                               const CursorPosition &cursorPosition,const QString &filePathToSave)
    {
        ConnectionSettings *connection = database->server()->connectionRecord();
        connection->setDefaultDatabase(database->name());
        return openShell(connection, ScriptInfo(script, execute, cursorPosition, shellName,filePathToSave));
    }

    MongoShell *App::openShell(ConnectionSettings *connection, const ScriptInfo &scriptInfo)
    {
        MongoServer *server = openServer(connection, false);
        MongoShell *shell = new MongoShell(server,scriptInfo);
        _shells.push_back(shell);
        _bus->publish(new OpeningShellEvent(this, shell));
        LOG_MSG("Opening shell...", mongo::LL_INFO);
        shell->execute();
        return shell;
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
