#include "robomongo/core/domain/App.h"
#include <QHash>

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/MongoShell.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/SshSettings.h"
#include "robomongo/core/mongodb/SshTunnelWorker.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/utils/StdUtils.h"
#include "robomongo/core/utils/Logger.h"

namespace Robomongo
{
    namespace detail
    {
        QString buildCollectionQuery(const std::string &collectionName, const QString &postfix)
        {
            QString qCollectionName = QtUtils::toQString(collectionName);

            QString pattern;

            // Use db.getCollection() to avoid having to enumerate and special case "reserved" names
            pattern = "db.getCollection(\'%1\').%2";

            // Escape '\' symbol
            qCollectionName.replace(QChar('\\'), "\\\\");

            return pattern.arg(qCollectionName).arg(postfix);
        }
    }

    App::App(EventBus *const bus) : QObject(),
        _bus(bus) {
        _bus->subscribe(this, EstablishSshConnectionResponse::Type);
    }

    App::~App()
    {
        std::for_each(_shells.begin(), _shells.end(), stdutils::default_delete<MongoShell*>());
        std::for_each(_servers.begin(), _servers.end(), stdutils::default_delete<MongoServer*>());
    }

    /**
     * Creates and opens new server connection.
     * @param connection: ConnectionSettings, that will be owned by MongoServer.
     * @param visible: should this server be visible in UI (explorer) or not.
     */
    MongoServer *App::openServer(ConnectionSettings *connection, bool visible) {
        if (visible)
            _bus->publish(new ConnectingEvent(this));

        // When connection is not "visible" or do not have
        // ssh settings enabled, continue without SSH Tunnel
        if (!visible || !connection->sshSettings()->enabled()) {
            return continueOpenServer(connection, visible);
        }

        // Open SSH channel and only after that open connection
        ConnectionSettings* settingsCopy = connection->clone();
        SshTunnelWorker* sshWorker = new SshTunnelWorker(settingsCopy);
        _bus->send(sshWorker, new EstablishSshConnectionRequest(this, sshWorker, settingsCopy, visible));
        return NULL;
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
        return openShell(connection, ScriptInfo(script, true, CursorPosition(0, -2), QtUtils::toQString(collection->database()->name()),filePathToSave));
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

    MongoServer* App::continueOpenServer(ConnectionSettings *connection, bool visible, int localport) {
        ConnectionSettings* settings = connection->clone();

        // Modify connection settings when SSH tunnel is used
        if (visible && settings->sshSettings()->enabled()) {
            settings->setServerHost("127.0.0.1");
            settings->setServerPort(localport);
        }

        MongoServer *server = new MongoServer(settings, visible);
        _servers.push_back(server);

        server->runWorkerThread();

        LOG_MSG(QString("Connecting to %1...").arg(QtUtils::toQString(server->connectionRecord()->getFullAddress())), mongo::logger::LogSeverity::Info());
        server->tryConnect();
        return server;
    }

    void App::handle(EstablishSshConnectionResponse *event) {
        if (event->isError()) {
            std::stringstream ss;
            ss << "Failed to create SSH tunnel to "
               << event->settings->sshSettings()->host() << ":"
               << event->settings->sshSettings()->port() << ".\n\nError:\n"
               << event->error().errorMessage();

            _bus->publish(new ConnectionFailedEvent(this, ss.str()));

            LOG_MSG(QString("Failed to create SSH tunnel: %1")
                .arg(QtUtils::toQString(event->error().errorMessage())), mongo::logger::LogSeverity::Error());

            event->worker->stopAndDelete();
            return;
        }

        LOG_MSG(QString("SSH tunnel created successfully"), mongo::logger::LogSeverity::Info());

        _bus->send(event->worker, new ListenSshConnectionRequest(this));
        continueOpenServer(event->settings, event->visible, event->localport);

        // record event->worker and delete when needed
    }


}
