#include "robomongo/core/mongodb/MongoWorker.h"

#include <QThread>

#include "mongo/client/global_conn_pool.h"
#include "mongo/client/replica_set_monitor.h"
#include <mongo/util/net/ssl_manager.h>
#include <mongo/util/net/ssl_options.h>

#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/engine/ScriptEngine.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/mongodb/MongoClient.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/ReplicaSetSettings.h"
#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/domain/MongoCollectionInfo.h"
#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/settings/SslSettings.h"
#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/core/utils/Logger.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    MongoWorker::MongoWorker(ConnectionSettings *connection, bool isLoadMongoRcJs, int batchSize,
                             int mongoTimeoutSec, int shellTimeoutSec, QObject *parent) : QObject(parent),
        _scriptEngine(nullptr),
        _isAdmin(true),
        _isLoadMongoRcJs(isLoadMongoRcJs),
        _batchSize(batchSize),
        _timerId(-1),
        _dbAutocompleteCacheTimerId(-1),
        _mongoTimeoutSec(mongoTimeoutSec),
        _shellTimeoutSec(shellTimeoutSec),
        _isQuiting(0),
        _dbclient(nullptr),
        _dbclientRepSet(nullptr),
        _connSettings(connection)
    {
        _thread = new QThread();
        moveToThread(_thread);
        VERIFY(connect( _thread, SIGNAL(finished()), _thread, SLOT(deleteLater()) ));
        VERIFY(connect( _thread, SIGNAL(finished()), this, SLOT(deleteLater()) ));
        _thread->start();
    }

    void MongoWorker::timerEvent(QTimerEvent *event)
    {
        if (_timerId == event->timerId()) {
            keepAlive();
            return;
        }

        if (_dbAutocompleteCacheTimerId == event->timerId() && !_scriptEngine) {
            _scriptEngine->invalidateDbCollectionsCache();
            return;
        }
    }

    void MongoWorker::keepAlive()
    {
        try {
            if (_dbclient) {
                // Building { ping: 1 }
                mongo::BSONObjBuilder command;
                command.append("ping", 1);
                mongo::BSONObj result;
                std::string authBase = getAuthBase();
                if (authBase.empty()) {
                    _dbclient->runCommand("admin", command.obj(), result);
                } else {
                    _dbclient->runCommand(authBase, command.obj(), result);
                }
            }

            if (_scriptEngine) {
                _scriptEngine->ping();
            }

        } catch(std::exception &ex) {
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::init()
    {        
        // todo: if (_initialized) return;
        try {
            _scriptEngine = new ScriptEngine(_connSettings, _shellTimeoutSec);  // todo: unique_ptr.reset()
            _scriptEngine->init(_isLoadMongoRcJs);
            _scriptEngine->use(_connSettings->defaultDatabase());
            _scriptEngine->setBatchSize(_batchSize);
            _timerId = startTimer(pingTimeMs);
            _dbAutocompleteCacheTimerId = startTimer(30000);
        } catch (const std::exception &ex) {
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::interrupt() {
        try {
            if (_isQuiting || !_scriptEngine)
                return;

            _scriptEngine->interrupt();
        } catch(const mongo::DBException &ex) {
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    MongoWorker::~MongoWorker()
    {
        if (_timerId != -1)
            killTimer(_timerId);

        if (_dbAutocompleteCacheTimerId != -1)
            killTimer(_dbAutocompleteCacheTimerId);

        delete _connSettings;
        delete _scriptEngine;

        // QThread "_thread" and MongoWorker itself will be deleted later
        // (see MongoWorker() constructor)
    }

    void MongoWorker::stopAndDelete()
    {
        _isQuiting = 1;
        _thread->quit();
    }

    /**
     * @brief Initiate connection to MongoDB
     */
    void MongoWorker::handle(EstablishConnectionRequest *event)
    {
        QMutexLocker lock(&_firstConnectionMutex);

        std::unique_ptr<ReplicaSet> repSetInfo(new ReplicaSet); // todo

        try {
            // Init MongoWorker
            init();
            
            mongo::DBClientBase *conn = getConnection(true);

            if (!conn) 
            {
                // Protection as default value: Logically/ideally, this error should never be seen.
                auto errorReason = std::string("Connection failure: Unknown error.");

                if (_connSettings->isReplicaSet()) {   
                    errorReason = "No member of the set is reachable.";

                    // Build ReplicaSet with no member reachable
                    std::vector<std::pair<std::string, bool>> membersAndHealths;
                    for (auto const& member : _connSettings->replicaSetSettings()->members()) {
                        membersAndHealths.push_back({ member, false });
                    }
                    repSetInfo.reset(new ReplicaSet(_connSettings->replicaSetSettings()->setName(), mongo::HostAndPort(),
                                                    membersAndHealths, "No member of the set is reachable."));

                    // todo: SSL case
                    // ...
                }
                else    // single server
                {
                    if (_connSettings->sslSettings()->sslEnabled())
                    {
                        // Note: Currently mongo-shell does not provide any interface to fetch actual error details
                        // for some SSL connection failures that's why we are unable to show exact error here. 
                        errorReason = "SSL tunnel failure: Network is unreachable or SSL connection rejected by server.";
                    }
                    else
                    {
                        errorReason = "Network is unreachable.";
                    }
                }

                resetGlobalSSLparams();

                reply(event->sender(), new EstablishConnectionResponse(this, EventError(errorReason), event->connectionType, 
                    *repSetInfo.release(), EstablishConnectionResponse::MongoConnection));

                return;
            }

            // There is no reachable primary with reachable secondary(ies) throw else primary is reachable so continue.
            if (_connSettings->isReplicaSet()) {
                ReplicaSet setInfo = getReplicaSetInfo();
                if (setInfo.primary.empty()) {  // No reachable primary, pass possible reachable secondary(ies)
                    repSetInfo.reset(new ReplicaSet(setInfo)); // todo: might be redundant
                    
                    reply(event->sender(), new RefreshReplicaSetResponse(this, setInfo, EventError(setInfo.errorStr)));
                    LOG_MSG(setInfo.errorStr, mongo::logger::LogSeverity::Error());
                    throw std::runtime_error(setInfo.errorStr);
                }
                else {  // Primary is reachable, save setInfo and continue
                    repSetInfo.reset(new ReplicaSet(setInfo.setName, setInfo.primary, setInfo.membersAndHealths));
                }
            }

            // For single server, connection is successful & For replica set, primary is reachable (=connection successful)
            if (_connSettings->hasEnabledPrimaryCredential()) {
                CredentialSettings *credentials = _connSettings->primaryCredential();

                // Building BSON object:
                mongo::BSONObj authParams(mongo::BSONObjBuilder()
                    .append("user", credentials->userName())
                    .append("db", credentials->databaseName())
                    .append("pwd", credentials->userPassword())
                    .append("mechanism", credentials->mechanism())
                    .obj());

                conn->auth(authParams);

                // If authentication succeed and database name is 'admin' -
                // then user is admin, otherwise user is not admin
                std::string dbName = credentials->databaseName();
                std::transform(dbName.begin(), dbName.end(), dbName.begin(), ::tolower);
                if (dbName.compare("admin") != 0) // dbName is NOT "admin"
                    _isAdmin = false;
            }

            boost::scoped_ptr<MongoClient> client(getClient());
            std::vector<std::string> dbNames = getDatabaseNamesSafe();

            // If we do not have databases, it means that we are unable to
            // execute "listdatabases" command and we have nothing to show.
            if (dbNames.size() == 0)
                throw mongo::DBException("Failed to execute \"listdatabases\" command.", 0);

            resetGlobalSSLparams();

            // todo: wrap rep. parameters into a struct
            auto connInfo = ConnectionInfo(_connSettings->getFullAddress(), dbNames, client->getVersion(), 
                                           client->getStorageEngineType());

            // todo: two ctors for rep.set and single server.
            reply(event->sender(), new EstablishConnectionResponse(this, connInfo, event->connectionType, 
                                                                   *repSetInfo.release()));
        } catch(const std::exception &ex) {
            resetGlobalSSLparams();

            auto errorReason = _connSettings->sslSettings()->sslEnabled() ?
                EstablishConnectionResponse::ErrorReason::MongoSslConnection : 
                EstablishConnectionResponse::ErrorReason::MongoAuth;

            reply(event->sender(), new EstablishConnectionResponse(this, EventError(ex.what()), event->connectionType, 
                *repSetInfo.release(), errorReason));
        }
    }

    void MongoWorker::handle(RefreshReplicaSetRequest *event)
    {
        ReplicaSet const replicaSetInfo = getReplicaSetInfo();

        if (replicaSetInfo.primary.empty()) { // Primary is unreachable, but there might have reachable secondary(ies) 
            reply(event->sender(), new RefreshReplicaSetResponse(this, replicaSetInfo, EventError(replicaSetInfo.errorStr)));
            LOG_MSG(replicaSetInfo.errorStr, mongo::logger::LogSeverity::Error());
            return;
        }

        // Primary is reachable
        reply(event->sender(), new RefreshReplicaSetResponse(this, replicaSetInfo));
    }

    std::string MongoWorker::getAuthBase() const
    {
        if (_connSettings->hasEnabledPrimaryCredential())
            return _connSettings->primaryCredential()->databaseName();

        return std::string();
    }

    MongoWorker::DatabasesContainerType MongoWorker::getDatabaseNamesSafe()
    {        
        DatabasesContainerType result;
        std::string authBase = getAuthBase();
        if (!_isAdmin && !authBase.empty()) {
            result.push_back(_connSettings->primaryCredential()->databaseName());
            return result;
        }

        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            result = client->getDatabaseNames();
        } catch(const std::exception &) {
            if (!authBase.empty())
                result.push_back(authBase);
        }
        return result;
    }

    /**
     * @brief Load list of all database names
     */
    void MongoWorker::handle(LoadDatabaseNamesRequest *event)
    {
        try {
            // If user not an admin - he doesn't have access to mongodb 'listDatabases' command
            // Non admin user has access only to the single database he specified while performing auth.
            std::vector<std::string> dbNames = getDatabaseNamesSafe();

            // Remove from list of created databases existing databases
            for (std::vector<std::string>::iterator it = dbNames.begin(); it != dbNames.end(); ++it) {
                std::unordered_set<std::string>::const_iterator exists = _createdDbs.find(*it);
                if (exists != _createdDbs.end()) {
                    _createdDbs.erase(*it);
                }
            }

            // Merge with list of created databases
            for (std::unordered_set<std::string>::iterator it = _createdDbs.begin(); it != _createdDbs.end(); ++it) {
                dbNames.push_back(*it);
            }

            if (dbNames.size()) {
                reply(event->sender(), new LoadDatabaseNamesResponse(this, dbNames));
            } else {
                reply(event->sender(), new LoadDatabaseNamesResponse(this, EventError("Failed to execute \"listdatabases\" command.")));
            }
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new LoadDatabaseNamesResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    /**
     * @brief Load list of all collection names
     */
    void MongoWorker::handle(LoadCollectionNamesRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());

            std::vector<std::string> stringList = client->getCollectionNames(event->databaseName());
            const std::vector<MongoCollectionInfo> &infos = client->runCollStatsCommand(stringList);
            client->done();

            reply(event->sender(), new LoadCollectionNamesResponse(this, event->databaseName(), infos));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new LoadCollectionNamesResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::handle(LoadUsersRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            const std::vector<MongoUser> &users = client->getUsers(event->databaseName());
            client->done();

            reply(event->sender(), new LoadUsersResponse(this, event->databaseName(), users));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new LoadUsersResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::handle(LoadCollectionIndexesRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            const std::vector<EnsureIndexInfo> &ind = client->getIndexes(event->collection());
            client->done();

            reply(event->sender(), new LoadCollectionIndexesResponse(this, ind));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new LoadCollectionIndexesResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::handle(EnsureIndexRequest *event)
    {
        const EnsureIndexInfo &newInfo = event->newInfo();
        const EnsureIndexInfo &oldInfo = event->oldInfo();
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->ensureIndex(oldInfo, newInfo);
            const std::vector<EnsureIndexInfo> &ind = client->getIndexes(newInfo._collection);
            client->done();

            reply(event->sender(), new LoadCollectionIndexesResponse(this, ind));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new LoadCollectionIndexesResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::handle(DropCollectionIndexRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->dropIndexFromCollection(event->collection(), event->name());
            client->done();
            reply(event->sender(), new DeleteCollectionIndexResponse(this, event->collection(), event->name()));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new DeleteCollectionIndexResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }            
    }

    void MongoWorker::handle(EditIndexRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->renameIndexFromCollection(event->collection(), event->oldIndex(), event->newIndex());
            const std::vector<EnsureIndexInfo> &ind = client->getIndexes(event->collection());
            client->done();

            reply(event->sender(), new LoadCollectionIndexesResponse(this, ind));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new LoadCollectionIndexesResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        } 
    }

    void MongoWorker::handle(LoadFunctionsRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            const std::vector<MongoFunction> &funs = client->getFunctions(event->databaseName());
            client->done();

            reply(event->sender(), new LoadFunctionsResponse(this, event->databaseName(), funs));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new LoadFunctionsResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::handle(InsertDocumentRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());

            if (event->overwrite())
                client->saveDocument(event->obj(), event->ns());
            else
                client->insertDocument(event->obj(), event->ns());

            client->done();

            reply(event->sender(), new InsertDocumentResponse(this));
        } catch(const mongo::DBException &ex) {
            EventError error = EventError("Error when saving document: " + ex.toString());
            reply(event->sender(), new InsertDocumentResponse(this, error));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::handle(RemoveDocumentRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());

            client->removeDocuments(event->ns(), event->query(), event->justOne());
            client->done();

            reply(event->sender(), new RemoveDocumentResponse(this));
        } catch(const mongo::DBException &ex) {
            EventError error = EventError("Error when deleting document: " + ex.toString());
            reply(event->sender(), new RemoveDocumentResponse(this, error));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::handle(ExecuteQueryRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            std::vector<MongoDocumentPtr> docs = client->query(event->queryInfo());
            client->done();

            reply(event->sender(), new ExecuteQueryResponse(this, event->resultIndex(), event->queryInfo(), docs));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new ExecuteQueryResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    /**
     * @brief Execute javascript
     */
    void MongoWorker::handle(ExecuteScriptRequest *event)
    {
        try {
            if (!_scriptEngine) {
                reply(event->sender(), new ExecuteScriptResponse(this, EventError("MongoDB Shell was not initialized")));
                return;
            }

            // If this is replica set, update script engine's server address
            if (_connSettings->isReplicaSet()) {
                // Refresh view of Replica Set Monitor to get latest status
                auto repSetMonitor = mongo::globalRSMonitorManager.getMonitor(_dbclientRepSet->getSetName());
                repSetMonitor->startOrContinueRefresh().refreshAll();

                // Update connection settings with primary(master) address and port
                mongo::HostAndPort repPrimary = repSetMonitor->getMasterOrUassert();

                // todo:
                // Update script engine with primary node
                _scriptEngine->init(_isLoadMongoRcJs, repPrimary.toString(), _connSettings->defaultDatabase());
            }

            // todo: should we use dbName from event or _connSettings? 
            MongoShellExecResult result = _scriptEngine->exec(event->script, _connSettings->defaultDatabase());

            reply(event->sender(), new ExecuteScriptResponse(this, result, event->script.empty()));
        } 
        catch(const mongo::DBException &ex) {
            reply(event->sender(), new ExecuteScriptResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    /**
     * @brief Interrupt javascript execution
     */
    void MongoWorker::handle(StopScriptRequest *)
    {
        try {
            if (!_scriptEngine) {
                return;
            }

            _scriptEngine->interrupt();
        } catch(const mongo::DBException &ex) {
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::handle(AutocompleteRequest *event)
    {
        try {
            if (!_scriptEngine) {
                reply(event->sender(), new AutocompleteResponse(this, EventError("MongoDB Shell was not initialized")));
                return;
            }

            QStringList list = _scriptEngine->complete(event->prefix, event->mode);
            reply(event->sender(), new AutocompleteResponse(this, list, event->prefix));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new AutocompleteResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::handle(CreateDatabaseRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->createDatabase(event->database());

            // Insert to list of created database
            // Read docs for this hashset in the header
            _createdDbs.insert(event->database());

            reply(event->sender(), new CreateDatabaseResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new CreateDatabaseResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::handle(DropDatabaseRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->dropDatabase(event->database());

            // Remove from the list of created database
            // Read docs for this hashset in the header
            _createdDbs.erase(event->database());

            reply(event->sender(), new DropDatabaseResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new DropDatabaseResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::handle(CreateCollectionRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->createCollection(event->getNs().toString(), event->getSize(), event->getCapped(),
                event->getMaxDocNum(), event->getExtraOptions());
            client->done();

            reply(event->sender(), new CreateCollectionResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new CreateCollectionResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::handle(DropCollectionRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->dropCollection(event->ns());
            client->done();

            reply(event->sender(), new DropCollectionResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new DropCollectionResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::handle(RenameCollectionRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->renameCollection(event->ns(), event->newCollection());
            client->done();

            reply(event->sender(), new RenameCollectionResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new RenameCollectionResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::handle(DuplicateCollectionRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->duplicateCollection(event->ns(), event->newCollection());
            client->done();

            reply(event->sender(), new DuplicateCollectionResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new DuplicateCollectionResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::handle(CopyCollectionToDiffServerRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            MongoWorker *cl = event->worker();
            client->copyCollectionToDiffServer(cl->_dbclient.get(), event->from(), event->to());
            client->done();

            reply(event->sender(), new CopyCollectionToDiffServerResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new CopyCollectionToDiffServerResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::handle(CreateUserRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->createUser(event->database(), event->user(), event->overwrite());
            client->done();

            reply(event->sender(), new CreateUserResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new CreateUserResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::handle(DropUserRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->dropUser(event->database(), event->id());
            client->done();

            reply(event->sender(), new DropUserResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new DropUserResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::handle(CreateFunctionRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->createFunction(event->database(), event->function(), event->existingFunctionName());
            client->done();

            reply(event->sender(), new CreateFunctionResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new CreateFunctionResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::handle(DropFunctionRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->dropFunction(event->database(), event->name());
            client->done();

            reply(event->sender(), new DropFunctionResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new DropFunctionResponse(this, EventError(ex.what())));
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    mongo::DBClientBase *MongoWorker::getConnection(bool mayReturnNull /* = false */)
    {
        // --- Configure SSL first --- // todo: move to function?
        // As a precaution reset SSL global params for any kind of connection request (SSL or non-SSL)
        resetGlobalSSLparams();
        // Update global SSL mode and global mongo SSL settings
        if (_connSettings->sslSettings()->sslEnabled())
        {
            // Force SSL mode for outgoing connections
            mongo::sslGlobalParams.sslMode.store(mongo::SSLParams::SSLMode_requireSSL);
            updateGlobalSSLparams();
        }
        else
        {
            // Disable forced SSL mode for outgoing connections
            mongo::sslGlobalParams.sslMode.store(mongo::SSLParams::SSLMode_allowSSL);
        }

        // --- Perform connection attempt ---
        if (_connSettings->isReplicaSet()) // connection to replica set 
        {  
            if (!_dbclientRepSet) 
            {
                // Step-1: Retrieve set name if we do not have it
                std::string setName = _connSettings->replicaSetSettings()->setName();
                // todo: move to func. "getSetName()"
                // If set name doesn't exist in the settings, get it from an on-line replica node
                if (setName.empty())
                {
                    auto dbclientTemp = upDBClientConnection(new mongo::DBClientConnection(true, 10));

                    // Try connecting to the nodes one by one until getting replica set name.
                    for (auto const& node : _connSettings->replicaSetSettings()->membersToHostAndPort())
                    {
                        mongo::Status const status = dbclientTemp->connect(node);
                        if (status.isOK())
                        {
                            _scriptEngine->init(_isLoadMongoRcJs, node.toString());
                            MongoShellExecResult result = _scriptEngine->exec("rs.status()", "");
                            if (!result.results().empty())
                            {
                                auto resultDocs = result.results().front().documents();
                                if (!resultDocs.empty())
                                {
                                    setName = resultDocs.front()->bsonObj().getStringField("set");
                                    if (!setName.empty()) // We get the information, finish the loop
                                        break;
                                }
                            }
                        }
                    }
                }

                if (setName.empty()) {  // It is not possible to continue with empty set name
                    return nullptr;
                }

                // Step-2: We have the set name, try connect to replica set
                auto const& membersHostsAndPorts = _connSettings->replicaSetSettings()->membersToHostAndPort();
                _dbclientRepSet = upDBClientReplicaSet(new mongo::DBClientReplicaSet(setName, membersHostsAndPorts, 
                                                       _mongoTimeoutSec));

                bool const connStatus = _dbclientRepSet->connect();
                
                if (!connStatus) {
                    return nullptr;
                }
            }
            return _dbclientRepSet.get();
        }
        else {                              // connection to single server
            if (!_dbclient) {
                // Timeout for operations
                // Connect timeout is fixed, but short, at 5 seconds (see headers for DBClientConnection)
                _dbclient = upDBClientConnection(new mongo::DBClientConnection(true, _mongoTimeoutSec));

                mongo::Status status = _dbclient->connect(_connSettings->info());

                if (!status.isOK() && mayReturnNull) {
                    return nullptr;
                }
            }
            return _dbclient.get();
        }
    }

    MongoClient *MongoWorker::getClient()
    {
        return new MongoClient(getConnection());
    }

    void MongoWorker::updateGlobalSSLparams() const
    {
        resetGlobalSSLparams();
        const SslSettings * const sslSettings = _connSettings->sslSettings();
        mongo::sslGlobalParams.sslAllowInvalidCertificates = sslSettings->allowInvalidCertificates();
        if (!mongo::sslGlobalParams.sslAllowInvalidCertificates)
        {
            mongo::sslGlobalParams.sslCAFile = sslSettings->caFile();
        }
        if (sslSettings->usePemFile())
        {
            mongo::sslGlobalParams.sslPEMKeyFile = sslSettings->pemKeyFile();
            mongo::sslGlobalParams.sslPEMKeyPassword = sslSettings->pemPassPhrase();
        }
        if (sslSettings->useAdvancedOptions())
        {
            mongo::sslGlobalParams.sslCRLFile = sslSettings->crlFile();
            mongo::sslGlobalParams.sslAllowInvalidHostnames = sslSettings->allowInvalidHostnames();
        }
    }

    void MongoWorker::resetGlobalSSLparams() const
    {
        mongo::sslGlobalParams.sslAllowInvalidCertificates = false;
        mongo::sslGlobalParams.sslCAFile = "";
        mongo::sslGlobalParams.sslPEMKeyFile = "";
        mongo::sslGlobalParams.sslPEMKeyPassword = "";
        mongo::sslGlobalParams.sslCRLFile = "";
        mongo::sslGlobalParams.sslAllowInvalidHostnames = false;
    }

    ReplicaSet MongoWorker::getReplicaSetInfo() const
    {
        std::string setName;
        mongo::HostAndPort primary;
        std::vector<std::pair<std::string, bool>> membersAndHealths;

        // Refresh view of Replica Set Monitor to get live data
        auto repSetMonitor = mongo::globalRSMonitorManager.getMonitor(_dbclientRepSet->getSetName());
        repSetMonitor->startOrContinueRefresh().refreshAll();

        setName = repSetMonitor->getName();
        auto const primaryOnly = mongo::ReadPreferenceSetting(mongo::ReadPreference::PrimaryOnly, mongo::TagSet());
        auto res = repSetMonitor->getHostOrRefresh(primaryOnly);
        if (res.isOK())
            primary = res.getValue();

        QStringList servers;
        // i.e. setNameAndMembers: "repset/localhost:27017,localhost:27018,localhost:27019"
        auto setNameAndMembers = QString::fromStdString(repSetMonitor->getServerAddress());
        QStringList result = setNameAndMembers.split("/");
        if (result.size() > 1) {
            servers = result[1].split(",");
        }

        // Save address and health of replica members
        for (QString const& server : servers) {
            auto hostAndPort = mongo::HostAndPort(mongo::StringData(server.toStdString()));
            membersAndHealths.push_back({ server.toStdString(), repSetMonitor->isHostUp(hostAndPort) });
        }

        return ReplicaSet(setName, primary, membersAndHealths, res.getStatus().reason());
    }

    /**
     * @brief Send event to this MongoWorker
     */
    void MongoWorker::send(Event *event)
    {
        if (_isQuiting)
            return;

        AppRegistry::instance().bus()->send(this, event);
    }

    /**
     * @brief Send reply event to object 'receiver'
     */
    void MongoWorker::reply(QObject *receiver, Event *event)
    {
        if (_isQuiting)
            return;

        AppRegistry::instance().bus()->send(receiver, event);
    }
}
