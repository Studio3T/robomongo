#include "robomongo/core/domain/MongoServer.h"

#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/SshSettings.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/settings/ReplicaSetSettings.h"
#include "robomongo/core/mongodb/MongoWorker.h"
#include "robomongo/core/mongodb/SshTunnelWorker.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/events/MongoEventsInfo.h"
#include "robomongo/core/utils/Logger.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/utils/common.h"

namespace Robomongo {
    R_REGISTER_EVENT(MongoServerLoadingDatabasesEvent)

    MongoServer::MongoServer(int handle, ConnectionSettings *settings, ConnectionType connectionType) : QObject(),
        _version(0.0f),
        _connectionType(connectionType),
        _worker(nullptr),
        _isConnected(false),
        _connSettings(settings),
        _handle(handle),
        _bus(AppRegistry::instance().bus()),
        _app(AppRegistry::instance().app()),
        _replicaSetInfo(nullptr)
    {}

    bool MongoServer::isConnected() const {
        return _isConnected;
    }

    ConnectionSettings *MongoServer::connectionRecord() const {
        return _connSettings.get();
    }

    MongoServer::~MongoServer() {
        clearDatabases();

        if (_worker) {
            _worker->stopAndDelete();
        }

        // MongoWorker "_worker" is not deleted here, because it is now owned by
        // another thread (call to moveToThread() made in MongoWorker constructor).
        // It will be deleted by this thread by means of "deleteLater()", which
        // is also specified in MongoWorker constructor.
    }

    /**
     * @brief Try to connect to MongoDB server.
     * @throws MongoException, if fails
     */
    void MongoServer::tryConnect() 
    {
        _bus->send(_worker, new EstablishConnectionRequest(this, _connectionType, _connSettings->uniqueId()));
    }

    void MongoServer::tryRefresh() 
    {
        _bus->send(_worker, new EstablishConnectionRequest(this, ConnectionRefresh, _connSettings->uniqueId()));
    }

    void MongoServer::tryRefreshReplicaSet()
    {
        _bus->send(_worker, new EstablishConnectionRequest(this, ConnectionRefresh, _connSettings->uniqueId()));
    }
    
    void MongoServer::tryRefreshReplicaSetFolder(bool showLoading /*= true*/)
    {
        if (!_connSettings->isReplicaSet())
            return;

        if (showLoading)
            _bus->publish(new ReplicaSetFolderLoading(this));

        _bus->send(_worker, new RefreshReplicaSetFolderRequest(this));
    }

    QStringList MongoServer::getDatabasesNames() const 
    {
        QStringList result;
        for (QList<MongoDatabase *>::const_iterator it = _databases.begin(); it != _databases.end(); ++it) {
            MongoDatabase *datab = *it;
            result.append(QtUtils::toQString(datab->name()));
        }
        return result;
    }

    void MongoServer::addDatabase(MongoDatabase *database) {
        _databases.append(database);
    }

    void MongoServer::createDatabase(const std::string &dbName) 
    {
        _bus->send(_worker, new CreateDatabaseRequest(this, dbName));
    }

    MongoDatabase *MongoServer::findDatabaseByName(const std::string &dbName) const 
    {
        for (QList<MongoDatabase *>::const_iterator it = _databases.begin(); it != _databases.end(); ++it) {
            MongoDatabase *datab = *it;
            if (datab->name() == dbName) {
                return datab;
            }
        }
        return NULL;
    }

    void MongoServer::dropDatabase(const std::string &dbName) {
        _bus->send(_worker, new DropDatabaseRequest(this, dbName));
    }

    void MongoServer::insertDocuments(const std::vector<mongo::BSONObj> &objCont,
                                      const MongoNamespace &ns) {
        for (std::vector<mongo::BSONObj>::const_iterator it = objCont.begin(); it != objCont.end(); it++) {
            insertDocument(*it, ns);
        }
    }

    void MongoServer::insertDocument(const mongo::BSONObj &obj, const MongoNamespace &ns) {
        _bus->send(_worker, new InsertDocumentRequest(this, obj, ns));
    }

    void MongoServer::saveDocuments(const std::vector<mongo::BSONObj> &objCont, const MongoNamespace &ns) {
        for (std::vector<mongo::BSONObj>::const_iterator it = objCont.begin(); it != objCont.end(); it++) {
            saveDocument(*it, ns);
        }
    }

    void MongoServer::saveDocument(const mongo::BSONObj &obj, const MongoNamespace &ns) {
        _bus->send(_worker, new InsertDocumentRequest(this, obj, ns, true));
    }

    void MongoServer::removeDocuments(mongo::Query query, const MongoNamespace &ns, bool justOne) {
        _bus->send(_worker, new RemoveDocumentRequest(this, query, ns, justOne));
    }

    void MongoServer::loadDatabases() 
    {
        _bus->publish(new MongoServerLoadingDatabasesEvent(this));
        if (_connSettings->isReplicaSet()) {
            tryRefreshReplicaSet();
        }
        else {  // single server
            _bus->send(_worker, new LoadDatabaseNamesRequest(this));
        }
    }

    void MongoServer::clearDatabases() 
    {
        qDeleteAll(_databases);
        _databases.clear();
    }

    void MongoServer::handle(EstablishConnectionResponse *event) 
    {
        _connectionType = event->connectionType;

        // In any case, replica set info must be updated, there might be reachable secondary(ies).
        // Also cached set name must be updated or cleared for failed connections.
        updateReplicaSetSettings(event);

        // --- Connection Failed
        if (event->isError()) {
            handleConnectionFailure(event);
            return;
        }

        // --- Connections Successful
        // Save various information after successful connection
        const ConnectionInfo &info = event->info;
        _version = info._version;
        _storageEngineType = info._storageEngineType;
        _isConnected = true;

        // ConnectionRefresh is used just to update connection view (_version, _storageEngineType, _repPrimary etc..)
        // So we return here after updating(refreshing) information related to connection view.
        if (ConnectionRefresh == event->connectionType) {
            if (_connSettings->isReplicaSet()) {
                // If it is replica set connection, do not return yet.
                LOG_MSG("Replica set refreshed. [" + _connSettings->connectionName() + ']', 
                         mongo::logger::LogSeverity::Info());
            }
            else {  // single server
                LOG_MSG("Server refreshed.", mongo::logger::LogSeverity::Info());
                return;
            }
        }

        // Only for single servers
        if (!_connSettings->isReplicaSet()) {
            _bus->publish(new ConnectionEstablishedEvent(this, _connectionType, info));
            // Do nothing if this is not a "primary" connection
            if (ConnectionPrimary != _connectionType)
                return;
        }

        if (ConnectionPrimary == _connectionType)
            LOG_MSG("Establish connection successful. Connection: " + _connSettings->connectionName(),
                     mongo::logger::LogSeverity::Info());

        clearDatabases();
        for (auto const& dbname : info._databases) {
            MongoDatabase *db  = new MongoDatabase(this, dbname);
            addDatabase(db);    // todo: serverClones for replica sets should not do this
        }

        if (_connSettings->isReplicaSet()) {
            _bus->publish(new ConnectionEstablishedEvent(this, event->connectionType, info));
            // In order to do first connection much faster, time consuming refresh 
            // "repSetMonitor->startOrContinueRefresh(). refreshAll()" is being requested after 
            // successful connection.
            if (ConnectionPrimary == event->connectionType)
                _bus->send(_worker, new RefreshReplicaSetFolderRequest(this));
        }
    }

    void MongoServer::handle(RefreshReplicaSetFolderResponse *event)
    {
        handleReplicaSetRefreshEvents(event->isError(), event->error(), event->replicaSet);
    }

    void MongoServer::handle(LoadDatabaseNamesResponse *event) 
    {
        if (event->isError()) {
            _bus->publish(new DatabaseListLoadedEvent(this, event->error()));
            return;
        }

        clearDatabases();

        for (auto const& dbname : event->databaseNames) {
            MongoDatabase *db  = new MongoDatabase(this, dbname);
            addDatabase(db);
        }

        _bus->publish(new DatabaseListLoadedEvent(this, _databases));
        LOG_MSG("Database list refreshed. Connection: " + _connSettings->connectionName(), 
                 mongo::logger::LogSeverity::Info());
    }

    void MongoServer::handle(InsertDocumentResponse *event) 
    {
        if (_connSettings->isReplicaSet()) // replica set
        {
            if (event->isError()) {
                if (ConnectionPrimary == _connectionType) { // Insert document from explorer context menu
                    if (EventError::SetPrimaryUnreachable == event->error().errorCode()) {
                        auto refreshEvent = ReplicaSetRefreshed(this, event->error(), event->error().replicaSetInfo());
                        handle(&refreshEvent);
                    }
                }
                else {  // Insert document from tab results window (Notifier, OutputWindow widget)
                    _bus->publish(new InsertDocumentResponse(this, event->error()));
                }
                genericResponseHandler(event, "Failed to insert document.", _bus, this);
                return;
            }
            _bus->publish(new InsertDocumentResponse(this));
            LOG_MSG("Document inserted.", mongo::logger::LogSeverity::Info());
        }
        else {  // single server
            _bus->publish(new InsertDocumentResponse(this, event->error()));
        }
    }

    void MongoServer::handle(RemoveDocumentResponse *event) 
    {
        _bus->publish(new RemoveDocumentResponse(event->sender(), event->error()));
    }

    void MongoServer::runWorkerThread() 
    {
        _worker = new MongoWorker(_connSettings->clone(),
                                  AppRegistry::instance().settingsManager()->loadMongoRcJs(),
                                  AppRegistry::instance().settingsManager()->batchSize(),
                                  AppRegistry::instance().settingsManager()->mongoTimeoutSec(),
                                  AppRegistry::instance().settingsManager()->shellTimeoutSec());
    }

    void MongoServer::handle(CreateDatabaseResponse *event) 
    {
        if (event->isError()) {
            if (_connSettings->isReplicaSet() &&
                EventError::SetPrimaryUnreachable == event->error().errorCode()) {
                auto refreshEvent = ReplicaSetRefreshed(this, event->error(), event->error().replicaSetInfo());
                handle(&refreshEvent);
            }
            genericResponseHandler(event, "Failed to create database \'" + event->database + "\'.", _bus, this);
        }
        else {
            loadDatabases();
            LOG_MSG("Database \'" + event->database + "\' created.", mongo::logger::LogSeverity::Info());
        }
    }

    void MongoServer::handle(DropDatabaseResponse *event) 
    {
        if (event->isError()) {
            if (_connSettings->isReplicaSet() &&
                EventError::SetPrimaryUnreachable == event->error().errorCode()) {
                auto refreshEvent = ReplicaSetRefreshed(this, event->error(), event->error().replicaSetInfo());
                handle(&refreshEvent);
            }
            genericResponseHandler(event, "Failed to drop database \'" + event->database + "\'.", _bus, this);
        }
        else {
            loadDatabases();
            LOG_MSG("Database \'" + event->database + "\' dropped.", mongo::logger::LogSeverity::Info());
        }
    }

    void MongoServer::handle(ReplicaSetRefreshed *event) 
    {
        handleReplicaSetRefreshEvents(event->isError(), event->error(), event->replicaSet);
    }

    void MongoServer::handleReplicaSetRefreshEvents(bool isError, EventError eventError, 
                                                    ReplicaSet const& replicaSet)
    {
        if (isError) { // Primary is unreachable
            _replicaSetInfo.reset(new ReplicaSet(replicaSet));
            LOG_MSG("Replica set folder refreshed with error. " + eventError.errorMessage() +
                    ". Connection: " + _connSettings->connectionName(), mongo::logger::LogSeverity::Error());
            _bus->publish(new ReplicaSetFolderRefreshed(this, eventError));
            return;
        }

        // Primary is reachable
        // Update replica set settings and mongo server _replicaSetInfo 
        _replicaSetInfo.reset(new ReplicaSet(replicaSet));
        _connSettings->setServerHost(_replicaSetInfo->primary.host());
        _connSettings->setServerPort(_replicaSetInfo->primary.port());
        _connSettings->replicaSetSettings()->setSetName(_replicaSetInfo->setName);

        LOG_MSG("Replica set folder refreshed. Connection: " + _connSettings->connectionName(),
                 mongo::logger::LogSeverity::Info());
        _bus->publish(new ReplicaSetFolderRefreshed(this));
    }

    void MongoServer::updateReplicaSetSettings(EstablishConnectionResponse* event)
    {
        if (!_connSettings->isReplicaSet())
            return;

        _replicaSetInfo.reset(new ReplicaSet(event->replicaSet));
        _connSettings->setServerHost(_replicaSetInfo->primary.host());
        _connSettings->setServerPort(_replicaSetInfo->primary.port());
        _connSettings->replicaSetSettings()->setSetName(_replicaSetInfo->setName);

        // Cache replica set name for 2 times faster first connection 
        if (ConnectionPrimary == _connectionType) {
            ConnectionSettings* originalConnSettings = AppRegistry::instance().settingsManager()
                ->getConnectionSettings(event->info._originalConnectionSettingsId);
            if (originalConnSettings) {
                originalConnSettings->replicaSetSettings()->setSetName(_replicaSetInfo->setName);
                AppRegistry::instance().settingsManager()->save();
                LOG_MSG("Replica set name cached as \"" + _replicaSetInfo->setName + "\".",
                    mongo::logger::LogSeverity::Info());
            }
            else
                LOG_MSG("Failed to cache the replica set name.", mongo::logger::LogSeverity::Warning());
        }
    }

    void MongoServer::handleConnectionFailure(EstablishConnectionResponse* event)
    {
        _isConnected = false;

        std::stringstream ss("Unknown error");
        auto eventErrorReason = event->errorReason;

        if (_connSettings->isReplicaSet())
        {
            ss.clear();
            std::string server = "";
            if (_connSettings->replicaSetSettings()->members().size() > 0)
                server = "[" + _connSettings->replicaSetSettings()->members().front() + "]";

            ss << "Cannot connect to replica set \"" << _connSettings->connectionName() << "\"" << server
                << ". \nSet's primary is unreachable.\n\nReason:\n" << event->error().errorMessage();

            _bus->publish(new ConnectionFailedEvent(this, _handle, event->connectionType, ss.str(),
                ConnectionFailedEvent::MongoConnection));
            _app->fireConnectionFailedEvent(_handle, event->connectionType, ss.str(),
                ConnectionFailedEvent::MongoConnection);
            LOG_MSG("Establish connection failed. " + event->error().errorMessage() + ". Connection: "
                + _connSettings->connectionName(), mongo::logger::LogSeverity::Error());
        }
        else    // single server 
        {
            if (EstablishConnectionResponse::ErrorReason::MongoSslConnection == eventErrorReason)
            {
                auto reason = ConnectionFailedEvent::SslConnection;
                ss.clear();
                ss << "Cannot connect to the MongoDB at " << connectionRecord()->getFullAddress()
                    << ".\n\nError:\n" << "SSL connection failure: " << event->error().errorMessage();
                _app->fireConnectionFailedEvent(_handle, _connectionType, ss.str(), reason);
            }
            else
            {
                auto reason = (EstablishConnectionResponse::ErrorReason::MongoAuth == eventErrorReason) ?
                    ConnectionFailedEvent::MongoAuth : ConnectionFailedEvent::MongoConnection;
                ss.clear();
                ss << "Cannot connect to the MongoDB at " << connectionRecord()->getFullAddress()
                    << ".\n\nError:\n" << event->error().errorMessage();
                _app->fireConnectionFailedEvent(_handle, _connectionType, ss.str(), reason);
            }

            // When connection cannot be established, we should cleanup this instance of MongoServer if it wasn't
            // shown in UI (i.e. it is not a Secondary connection that is used for shells tab)
            if (_connectionType == ConnectionPrimary || _connectionType == ConnectionTest)
            {
                LOG_MSG("Establish connection failed. " + event->error().errorMessage() +
                    ". Connection: " + _connSettings->connectionName(),
                    mongo::logger::LogSeverity::Error());
                _app->closeServer(this);
            }
        }
    }

}   // namespace Robomongo
