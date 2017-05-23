#include "robomongo/core/mongodb/MongoWorker.h"

#include <algorithm>

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
#include "robomongo/utils/string_operations.h"

namespace Robomongo
{
    std::string const PRIMARY_UNREACHABLE = "Replica set's primary is unreachable.";

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
                pingDatabase(_dbclient.get());
            }

            if (_dbclientRepSet) {
                pingDatabase(_dbclientRepSet.get());
            }

            if (_scriptEngine) {
                _scriptEngine->ping();
            }

        } catch(std::exception &ex) {
            LOG_MSG("Failed to ping the server. MongoWorker::keepAlive() failed. " + std::string(ex.what()), 
                    mongo::logger::LogSeverity::Error());
        }
    }

    void MongoWorker::init()
    {        
        try {
            _scriptEngine.reset(new ScriptEngine(_connSettings, _shellTimeoutSec));
            _scriptEngine->init(_isLoadMongoRcJs);
            _scriptEngine->use(_connSettings->defaultDatabase());
            _scriptEngine->setBatchSize(_batchSize);
            _timerId = startTimer(pingTimeMs);
            _dbAutocompleteCacheTimerId = startTimer(30000);
        } catch (const std::exception &ex) {
            LOG_MSG("Failed to initialize MongoWorker. " + std::string(ex.what()), 
                     mongo::logger::LogSeverity::Error());
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

        // QThread "_thread" and MongoWorker itself will be deleted later
        // (see MongoWorker() constructor)
    }

    void MongoWorker::stopAndDelete()
    {
        _isQuiting = 1;
        _thread->quit();
    }

    void MongoWorker::changeTimeout(int newTimeout)
    {
        _scriptEngine->changeTimeout(newTimeout);
    }

    /**
     * @brief Initiate connection to MongoDB
     */
    bool MongoWorker::handle(EstablishConnectionRequest *event)
    {
        QMutexLocker lock(&_firstConnectionMutex);

        std::unique_ptr<ReplicaSet> repSetInfo(new ReplicaSet);
        auto errorCode = EventError::ErrorCode::Unknown;

        try {
            mongo::DBClientBase *conn = getConnection(true);
            
            // --- Connection failed for single server & replica set (no member of the set is reachable)
            if (!conn) 
            {
                auto errorReason = std::string("Connection failure: Unknown error.");

                if (_connSettings->sslSettings()->sslEnabled()) {
                    // Note: Currently mongo-shell does not provide any interface to fetch actual error details
                    // for some SSL connection failures that's why we are unable to show exact error here. 
                    errorReason = "SSL tunnel failure: Network is unreachable or SSL connection rejected by server.";
                }
                else {  // Non-SSL connections
                    if (_connSettings->isReplicaSet()) {
                        errorReason = "No member of the set is reachable.";
                        std::vector<std::pair<std::string, bool>> membersAndHealths;
                        for (auto const& member : _connSettings->replicaSetSettings()->members()) {
                            membersAndHealths.push_back({ member, false });
                        }
                        repSetInfo.reset(new ReplicaSet("", mongo::HostAndPort(), membersAndHealths,
                                                        "No member of the set is reachable."));
                    }
                    else    // single server
                        errorReason = "Network is unreachable.";                    
                }
                resetGlobalSSLparams();

                reply(event->sender(), new EstablishConnectionResponse(this, EventError(errorReason, errorCode),             
                      event->connectionType, event->uuid, *repSetInfo.release(), 
                      EstablishConnectionResponse::MongoConnection));

                return false;
            }

            // --- Single server: Connection successful 
            // --- Replica set:   Connection successful (primary reachable) or 
            //                    Connection failed (primary unreachable with at least one reachable member)
            if (_connSettings->isReplicaSet()) {
                bool refresh = (ConnectionType::ConnectionRefresh == event->connectionType);
                ReplicaSet const& setInfo = getReplicaSetInfo(refresh);

                // Check if same set name used with different members which is not supported
                auto const& members = _connSettings->replicaSetSettings()->members();
                if (std::find(members.cbegin(), members.cend(), setInfo.primary.toString()) == members.cend()) 
                {   // primary not found between user entered members
                    std::string const errorStr = "Different members found under same replica set name \"" + 
                         setInfo.setName + "\".";
                    repSetInfo.reset(new ReplicaSet(setInfo));
                    errorCode = EventError::ErrorCode::ServerHasDifferentMembers;
                    LOG_MSG(errorStr, mongo::logger::LogSeverity::Error());
                    throw std::runtime_error(errorStr);
                }

                if (setInfo.primary.empty()) {  // No reachable primary
                    repSetInfo.reset(new ReplicaSet(setInfo)); // Pass possible reachable secondary(ies) info 
                    LOG_MSG(setInfo.errorStr, mongo::logger::LogSeverity::Error());
                    throw std::runtime_error(setInfo.errorStr);
                }
                else {  // Primary is reachable, save setInfo and continue
                    repSetInfo.reset(new ReplicaSet(setInfo));
                }
            }

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

            if (!_connSettings->isReplicaSet())
                init(); // Init MongoWorker for single server (for replica set connections early init is used)

            resetGlobalSSLparams();

            auto connInfo = ConnectionInfo(_connSettings->getFullAddress(), dbNames, client->getVersion(), 
                                           client->getStorageEngineType(), event->uuid);

            // todo: two ctors for rep.set and single server.
            reply(event->sender(), new EstablishConnectionResponse(this, connInfo, event->connectionType, 
                                                                   *repSetInfo.release()));
            return true;
        } 
        catch(const std::exception &ex) {
            resetGlobalSSLparams();
            auto errorReason = _connSettings->sslSettings()->sslEnabled() ?
                               EstablishConnectionResponse::ErrorReason::MongoSslConnection : 
                               EstablishConnectionResponse::ErrorReason::MongoAuth;

            reply(event->sender(), new EstablishConnectionResponse(this, EventError(ex.what(), errorCode), 
                  event->connectionType, ConnectionInfo(event->uuid), *repSetInfo.release(), errorReason));
        }

        return false;
    }

    void MongoWorker::handle(RefreshReplicaSetFolderRequest *event)
    {
		configureSSL();
        ReplicaSet const& replicaSetInfo = getReplicaSetInfo(true);

        // Primary is unreachable, but there might be reachable secondary(ies)
        if (replicaSetInfo.primary.empty()) {  
            reply(event->sender(), new RefreshReplicaSetFolderResponse(this, replicaSetInfo, event->expanded,
                                                                       EventError(replicaSetInfo.errorStr)));
            LOG_MSG(replicaSetInfo.errorStr, mongo::logger::LogSeverity::Error());
            return;
        }
        else // Primary is reachable
            reply(event->sender(), new RefreshReplicaSetFolderResponse(this, replicaSetInfo, event->expanded));
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

            auto const& namespaces = client->getCollectionNamesWithDbname(event->databaseName());
            std::vector<MongoCollectionInfo> const& collInfos = client->runCollStatsCommand(namespaces);
            client->done();

            reply(event->sender(), new LoadCollectionNamesResponse(this, event->databaseName(), collInfos));
        } catch(const mongo::DBException &ex) {
            if (_connSettings->isReplicaSet()) {
                ReplicaSet const& replicaSetInfo = getReplicaSetInfo(true);
                if (replicaSetInfo.primary.empty()) {  // primary not reachable
                    reply(event->sender(), new LoadCollectionNamesResponse(this,
                          EventError(PRIMARY_UNREACHABLE, replicaSetInfo, false)));
                }
                else // other errors
                    reply(event->sender(), new LoadCollectionNamesResponse(this, EventError(ex.what())));
            }
            else // single server
                reply(event->sender(), new LoadCollectionNamesResponse(this, EventError(ex.what())));
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
            if (_connSettings->isReplicaSet()) {
                ReplicaSet const& replicaSetInfo = getReplicaSetInfo(true);
                if (replicaSetInfo.primary.empty()) {  // primary not reachable
                    reply(event->sender(), new LoadUsersResponse(this,
                          EventError(PRIMARY_UNREACHABLE, replicaSetInfo, false)));
                }
                else // other errors
                    reply(event->sender(), new LoadUsersResponse(this, EventError(ex.what())));
            }
            else // single server
                reply(event->sender(), new LoadUsersResponse(this, EventError(ex.what())));
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
            const std::vector<MongoFunction> &funcs = client->getFunctions(event->databaseName());
            client->done();

            // If list of functions from client is empty, try getting it with script engine
            if (funcs.empty()) {
                MongoShellExecResult const& result = _scriptEngine->exec("db.system.js.find()", event->databaseName());
                std::vector<MongoFunction> functions;
                if (!result.results().empty()) {
                    auto const& resultDocs = result.results().front().documents();
                    for (auto const res : resultDocs)
                        functions.push_back(MongoFunction(res->bsonObj()));
                }
                reply(event->sender(), new LoadFunctionsResponse(this, event->databaseName(), functions));
                return;
            }

            reply(event->sender(), new LoadFunctionsResponse(this, event->databaseName(), funcs));
        } catch(const mongo::DBException &ex) {
            if (_connSettings->isReplicaSet()) {
                ReplicaSet const& replicaSetInfo = getReplicaSetInfo(true);
                if (replicaSetInfo.primary.empty()) {  // primary not reachable
                    reply(event->sender(), new LoadFunctionsResponse(this,
                          EventError(PRIMARY_UNREACHABLE, replicaSetInfo, false)));
                }
                else // other errors
                    reply(event->sender(), new LoadFunctionsResponse(this, EventError(ex.what())));
            }
            else // single server
                reply(event->sender(), new LoadFunctionsResponse(this, EventError(ex.what())));
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
        } 
        catch(const mongo::DBException &ex) {
            if (_connSettings->isReplicaSet()) {
                ReplicaSet const& replicaSetInfo = getReplicaSetInfo(true);
                if (replicaSetInfo.primary.empty()) {  // primary not reachable
                    reply(event->sender(), new InsertDocumentResponse(this,
                          EventError(PRIMARY_UNREACHABLE, replicaSetInfo, false)));
                }
                else    // other errors
                    reply(event->sender(), new InsertDocumentResponse(this, EventError(ex.toString())));
            }
            else { // single server
                    reply(event->sender(), new InsertDocumentResponse(this, 
                          EventError("Error when saving document: " + ex.toString())));
            }
        }
    }

    void MongoWorker::handle(RemoveDocumentRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());

            client->removeDocuments(event->ns(), event->query(), (event->removeCount() == RemoveDocumentCount::ONE));
            client->done();

            reply(event->sender(), new RemoveDocumentResponse(this, event->removeCount(), event->index()));
        } 
        catch(const mongo::DBException &ex) {
            if (_connSettings->isReplicaSet()) {
                ReplicaSet const& replicaSetInfo = getReplicaSetInfo(true);
                if (replicaSetInfo.primary.empty()) {  // primary not reachable
                    reply(event->sender(), new RemoveDocumentResponse(this,
                          EventError(PRIMARY_UNREACHABLE, replicaSetInfo, false), event->removeCount(), event->index()));
                }
                else // other errors
                    reply(event->sender(), new RemoveDocumentResponse(this, EventError(ex.what()), 
                          event->removeCount(), event->index()));
            }
            else // single server
                reply(event->sender(), new RemoveDocumentResponse(this,
                      EventError("Error when deleting document: " + ex.toString()), event->removeCount(), event->index()));
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

            // Try to handle case where new shell (which was opened when server unreachable) was re-executed
            if (_scriptEngine->failedScope()) {
                try {
                    _scriptEngine->init(_isLoadMongoRcJs);
                }
                catch (std::exception const& ex) {     
                    LOG_MSG(captilizeFirstChar(ex.what()) + ", cannot init mongo scope", 
                            mongo::logger::LogSeverity::Error());
                }
            }

            // todo: should we use dbName from event or _connSettings? 
            MongoShellExecResult result = _scriptEngine->exec(event->script, _connSettings->defaultDatabase());

            // To fix the problem where 'result' comes with old primary address.
            if (_connSettings->isReplicaSet()) 
                result.setCurrentServer(_dbclientRepSet->getSuspectedPrimaryHostAndPort().toString());

            // Robomongo shell timeout
            bool timeoutReached = false;
            if (result.timeoutReached()) 
                timeoutReached = true;          

            if (result.error()) {
                // If this is replica set, update script engine and try again
                if (_connSettings->isReplicaSet()) {
                    ReplicaSet const& replicaSetInfo = getReplicaSetInfo(true);
                    if (replicaSetInfo.primary.empty()) {  // primary not reachable
                        reply(event->sender(), 
                            new ExecuteScriptResponse(this, EventError(PRIMARY_UNREACHABLE, replicaSetInfo, false)));
                        return;
                    }
                    else {  // primary reachable
                        _scriptEngine->init(_isLoadMongoRcJs, replicaSetInfo.primary.toString(),
                                            _connSettings->defaultDatabase());
                        result = _scriptEngine->exec(event->script, _connSettings->defaultDatabase());
                    }
                }
                else { // single server
                    reply(event->sender(), 
                        new ExecuteScriptResponse(this, EventError(result.errorMessage())));
                    return;
                }
            }

            reply(event->sender(), new ExecuteScriptResponse(this, result, event->script.empty(), timeoutReached));
        } 
        catch(const std::exception &ex) {
            reply(event->sender(), new ExecuteScriptResponse(this, EventError(ex.what(), EventError::Unknown, false)));
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

            // Insert to list of created database. Read docs for this hashset in the header
            _createdDbs.insert(event->database());

            reply(event->sender(), new CreateDatabaseResponse(this, event->database()));
        } catch(const mongo::DBException &ex) {
            if (_connSettings->isReplicaSet()) {
                ReplicaSet const& replicaSetInfo = getReplicaSetInfo(true);
                if (replicaSetInfo.primary.empty()) {  // primary not reachable
                    reply(event->sender(), new CreateDatabaseResponse(this, event->database(),
                          EventError(PRIMARY_UNREACHABLE, replicaSetInfo, false)));
                }
                else    // other errors
                    reply(event->sender(), new CreateDatabaseResponse(this, event->database(), 
                                                                      EventError(ex.toString())));
            }
            else  // single server
                reply(event->sender(), new CreateDatabaseResponse(this, event->database(), EventError(ex.what())));
        }
    }

    void MongoWorker::handle(DropDatabaseRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->dropDatabase(event->database);

            // Remove from the list of created database, Read docs for this hashset in the header
            _createdDbs.erase(event->database);

            reply(event->sender(), new DropDatabaseResponse(this, event->database));
        } 
        catch(const mongo::DBException &ex) {
            if (_connSettings->isReplicaSet()) {
                ReplicaSet const& replicaSetInfo = getReplicaSetInfo(true);
                if (replicaSetInfo.primary.empty()) {  // primary not reachable
                    reply(event->sender(), new DropDatabaseResponse(this, event->database,
                          EventError(PRIMARY_UNREACHABLE, replicaSetInfo, false)));
                }
                else // other errors
                    reply(event->sender(), new DropDatabaseResponse(this, event->database, EventError(ex.toString())));
            }
            else  // single server
                reply(event->sender(), new DropDatabaseResponse(this, event->database, EventError(ex.what())));
        }
    }

    void MongoWorker::handle(CreateCollectionRequest *event)
    {
        std::string const& collection = event->ns().collectionName();

        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->createCollection(event->ns().toString(), event->getSize(), event->getCapped(),
                event->getMaxDocNum(), event->getExtraOptions());
            client->done();

            reply(event->sender(), new CreateCollectionResponse(this, collection));
        } catch(const mongo::DBException &ex) {
            if (_connSettings->isReplicaSet()) {
                ReplicaSet const& replicaSetInfo = getReplicaSetInfo(true);
                if (replicaSetInfo.primary.empty()) {  // primary not reachable
                    reply(event->sender(), new CreateCollectionResponse(this, collection,
                          EventError(PRIMARY_UNREACHABLE, replicaSetInfo, false)));
                }
                else // other errors
                    reply(event->sender(), new CreateCollectionResponse(this, collection, EventError(ex.toString())));
            }
            else // single server
                reply(event->sender(), new CreateCollectionResponse(this, collection, EventError(ex.what())));
        }
    }

    void MongoWorker::handle(DropCollectionRequest *event)
    {
        std::string const& collection = event->ns().collectionName();

        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->dropCollection(event->ns());
            client->done();

            reply(event->sender(), new DropCollectionResponse(this, collection));
        } catch(const mongo::DBException &ex) {
            if (_connSettings->isReplicaSet()) {
                ReplicaSet const& replicaSetInfo = getReplicaSetInfo(true);
                if (replicaSetInfo.primary.empty()) {  // primary not reachable
                    reply(event->sender(), new DropCollectionResponse(this, collection,
                          EventError(PRIMARY_UNREACHABLE, replicaSetInfo, false)));
                }
                else // other errors
                    reply(event->sender(), new DropCollectionResponse(this, collection, EventError(ex.toString())));
            }
            else // single server
                reply(event->sender(), new DropCollectionResponse(this, collection, EventError(ex.what())));
        }
    }

    void MongoWorker::handle(RenameCollectionRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->renameCollection(event->ns(), event->newCollection());
            client->done();

            reply(event->sender(), new RenameCollectionResponse(this, event->ns().collectionName(),
                                                                event->newCollection()));
        } catch(const mongo::DBException &ex) {
            if (_connSettings->isReplicaSet()) {
                ReplicaSet const& replicaSetInfo = getReplicaSetInfo(true);
                if (replicaSetInfo.primary.empty()) {  // primary not reachable
                    reply(event->sender(), new RenameCollectionResponse(this,
                          EventError(PRIMARY_UNREACHABLE, replicaSetInfo, false)));
                }
                else // other errors
                    reply(event->sender(), new RenameCollectionResponse(this, EventError(ex.toString())));
            }
            else // single server
                reply(event->sender(), new RenameCollectionResponse(this, EventError(ex.what())));
        }
    }

    void MongoWorker::handle(DuplicateCollectionRequest *event)
    {
        std::string const& sourceCollection = event->ns().collectionName();

        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->duplicateCollection(event->ns(), event->newCollection());
            client->done();

            reply(event->sender(), new DuplicateCollectionResponse(this, sourceCollection, event->newCollection()));
        }
        catch (const mongo::DBException &ex) {
            if (_connSettings->isReplicaSet()) {
                ReplicaSet const& replicaSetInfo = getReplicaSetInfo(true);
                if (replicaSetInfo.primary.empty()) {  // primary not reachable
                    reply(event->sender(), new DuplicateCollectionResponse(this, sourceCollection,
                          EventError(PRIMARY_UNREACHABLE, replicaSetInfo, false)));
                }
                else    // other errors
                    reply(event->sender(), new DuplicateCollectionResponse(this, sourceCollection, EventError(ex.what())));
            }
            else  // single server
                reply(event->sender(), new DuplicateCollectionResponse(this, sourceCollection, EventError(ex.what())));
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

            reply(event->sender(), new CreateUserResponse(this, event->user().name()));
        } catch(const mongo::DBException &ex) {
            if (_connSettings->isReplicaSet()) {
                ReplicaSet const& replicaSetInfo = getReplicaSetInfo(true);
                if (replicaSetInfo.primary.empty()) {  // primary not reachable
                    reply(event->sender(), new CreateUserResponse(this, event->user().name(),
                          EventError(PRIMARY_UNREACHABLE, replicaSetInfo, false)));
                }
                else   // other errors
                    reply(event->sender(), new CreateUserResponse(this, event->user().name(), 
                                                                  EventError(ex.what())));
            }
            else  // single server
                reply(event->sender(), new CreateUserResponse(this, event->user().name(), 
                                                              EventError(ex.what())));
        }
    }

    void MongoWorker::handle(DropUserRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->dropUser(event->database(), event->id());
            client->done();

            reply(event->sender(), new DropUserResponse(this, event->username()));
        } catch(const mongo::DBException &ex) {
            if (_connSettings->isReplicaSet()) {
                ReplicaSet const& replicaSetInfo = getReplicaSetInfo(true);
                if (replicaSetInfo.primary.empty()) {  // primary not reachable
                    reply(event->sender(), new DropUserResponse(this, event->username(),
                          EventError(PRIMARY_UNREACHABLE, replicaSetInfo, false)));
                }
                else   // other errors
                    reply(event->sender(), new DropUserResponse(this, event->username(), EventError(ex.what())));
            }
            else  // single server
                reply(event->sender(), new DropUserResponse(this, event->username(), EventError(ex.what())));
        }
    }

    void MongoWorker::handle(CreateFunctionRequest *event)
    {
        std::string const& functionName = event->function().name();

        try {
            if (event->dbVersion() >= 3.4) {
                auto const cmd = "db.system.js.save(" + event->function().toBson().toString() + ')';
                MongoShellExecResult const& result = _scriptEngine->exec(cmd, event->database());
                if (result.error())
                    throw mongo::DBException(result.errorMessage(), 0);
            }
            else {
                boost::scoped_ptr<MongoClient> client(getClient());
                client->createFunction(event->database(), event->function(), event->existingFunctionName());
                client->done();
            }

            reply(event->sender(), new CreateFunctionResponse(this, functionName));
        } catch(const mongo::DBException &ex) {
            if (_connSettings->isReplicaSet()) {
                ReplicaSet const& replicaSetInfo = getReplicaSetInfo(true);
                if (replicaSetInfo.primary.empty()) {  // primary not reachable
                    reply(event->sender(), new CreateFunctionResponse(this, functionName,
                          EventError(PRIMARY_UNREACHABLE, replicaSetInfo, false)));
                }
                else    // other errors
                    reply(event->sender(), new CreateFunctionResponse(this, functionName, EventError(ex.what())));
            }
            else  // single server
                reply(event->sender(), new CreateFunctionResponse(this, functionName, EventError(ex.what())));
        }
    }

    void MongoWorker::handle(DropFunctionRequest *event)
    {
        try {
            if (event->dbVersion() >= 3.4) {
                auto const cmd = "db.system.js.remove( { _id : \"" + event->functionName() + "\" } )";
                MongoShellExecResult const& result = _scriptEngine->exec(cmd, event->database());
                if (result.error())
                    throw mongo::DBException(result.errorMessage(), 0);
            }
            else {
                boost::scoped_ptr<MongoClient> client(getClient());
                client->dropFunction(event->database(), event->functionName());
                client->done();
            }

            reply(event->sender(), new DropFunctionResponse(this, event->functionName()));
        }
        catch (const mongo::DBException &ex) {
            if (_connSettings->isReplicaSet()) {
                ReplicaSet const& replicaSetInfo = getReplicaSetInfo(true);
                if (replicaSetInfo.primary.empty()) {  // primary not reachable
                    reply(event->sender(), new DropFunctionResponse(this, event->functionName(),
                        EventError(PRIMARY_UNREACHABLE, replicaSetInfo, false)));
                }
                else  // other errors
                    reply(event->sender(), new DropFunctionResponse(this, event->functionName(), EventError(ex.what())));
            }
            else  // single server
                reply(event->sender(), new DropFunctionResponse(this, event->functionName(), EventError(ex.what())));
        }
    }

    mongo::DBClientBase *MongoWorker::getConnection(bool mayReturnNull /* = false */)
    {
        configureSSL();

        // --- Perform connection ---
        if (_connSettings->isReplicaSet()) // connection to replica set 
        {  
            if (!_dbclientRepSet) 
            {
                init(); // Init mongoworker for early-use of _scriptEngine

                // Step-1: Use user entered set name or retrieve set name from cache or from a reachable member
                std::string setName = _connSettings->replicaSetSettings()->setNameUserEntered();
                if (setName.empty()) {
                    setName = _connSettings->replicaSetSettings()->cachedSetName();
                    if (setName.empty()) // If there is no cached set name, get it from an on-line replica node
                        setName = connectAndGetReplicaSetName();

                    if (setName.empty())   // It is not possible to continue with empty set name
                        return nullptr;
                }

                // Step-2: Try connect to replica set with set name
                auto const& membersHostsAndPorts = _connSettings->replicaSetSettings()->membersToHostAndPort();
                _dbclientRepSet = DBClientReplicaSet(new mongo::DBClientReplicaSet(setName, membersHostsAndPorts, 
                                                     "Robomongo", _mongoTimeoutSec));
                
                if (!_dbclientRepSet->connect()) 
                    return nullptr;
            }
            return _dbclientRepSet.get();
        }
        else {  // connection to single server
            if (!_dbclient) {
                // Timeout for operations
                // Connect timeout is fixed, but short, at 5 seconds (see headers for DBClientConnection)
                _dbclient = DBClientConnection(new mongo::DBClientConnection(true, _mongoTimeoutSec));

                mongo::Status status = _dbclient->connect(_connSettings->hostAndPort(), "Robomongo");
                if (!status.isOK() && mayReturnNull) 
                    return nullptr;
            }
            return _dbclient.get();
        }
    }

    MongoClient *MongoWorker::getClient()
    {
        return new MongoClient(getConnection());
    }

    void MongoWorker::configureSSL()
    {
        // As a precaution reset SSL global params for any kind of connection request (SSL or non-SSL)
        resetGlobalSSLparams();
        // Update global SSL mode and global mongo SSL settings
        if (_connSettings->sslSettings()->sslEnabled()) {
            // Force SSL mode for outgoing connections
            mongo::sslGlobalParams.sslMode.store(mongo::SSLParams::SSLMode_requireSSL);
            updateGlobalSSLparams();
        }
        else {
            // Disable forced SSL mode for outgoing connections
            mongo::sslGlobalParams.sslMode.store(mongo::SSLParams::SSLMode_allowSSL);
        }
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

    ReplicaSet MongoWorker::getReplicaSetInfo(bool refresh /*= true*/) const
    {
        std::string setName;
        mongo::HostAndPort primary;
        std::vector<std::pair<std::string, bool>> membersAndHealths;

        // Refresh view of Replica Set Monitor to get live data
        auto repSetMonitor = mongo::globalRSMonitorManager.getMonitor(_dbclientRepSet->getSetName());
        
        // Handle long lasting (e.g. overnight) connections which causes replica set monitor to be null
        if (!repSetMonitor) {
            mongo::ReplicaSetMonitor::createIfNeeded(_dbclientRepSet->getSetName(),
                                                     _connSettings->replicaSetSettings()->membersToHostAndPortAsSet());
            repSetMonitor = mongo::globalRSMonitorManager.getMonitor(_dbclientRepSet->getSetName());
            if (!repSetMonitor) // if nullptr again even if the set is reachable, something went really wrong.
                return ReplicaSet(setName, primary, membersAndHealths, 
                                  "Unexpected error. Unable to get replica set monitor for set name: " + 
                                  _dbclientRepSet->getSetName() + "\nPlease open a new connection.");
        }

        // refreshAll() takes long time, in order to do first establish connection request much faster
        // refreshAll() will be by-passed first and it will/must be done after a successful connection.
        if (refresh)
            repSetMonitor->startOrContinueRefresh().refreshAll();

        setName = repSetMonitor->getName();
        auto const primaryOnly = mongo::ReadPreferenceSetting(mongo::ReadPreference::PrimaryOnly, mongo::TagSet());
        auto primaryWithStatus = repSetMonitor->getHostOrRefresh(primaryOnly, mongo::Milliseconds(2000)); // todo
        if (primaryWithStatus.isOK())
            primary = primaryWithStatus.getValue();

        QStringList servers;
        // i.e. setNameAndMembers: "repset/localhost:27017,localhost:27018,localhost:27019"
        auto setNameAndMembers = QString::fromStdString(repSetMonitor->getServerAddress());
        QStringList result = setNameAndMembers.split("/");
        if (result.size() > 1) 
            servers = result[1].split(",");

        // Save address and health of replica members
        for (QString const& server : servers) {
            auto hostAndPort = mongo::HostAndPort(mongo::StringData(server.toStdString()));
            membersAndHealths.push_back({ server.toStdString(), repSetMonitor->isHostUp(hostAndPort) });
        }

        return ReplicaSet(setName, primary, membersAndHealths, primaryWithStatus.getStatus().reason());
    }

    std::string MongoWorker::connectAndGetReplicaSetName() const
    {
        auto dbclientTemp = DBClientConnection(new mongo::DBClientConnection(true, 10));
        std::string setName = "";

        // Try connecting to the nodes one by one until getting replica set name.
        for (auto const& node : _connSettings->replicaSetSettings()->membersToHostAndPort())
        {
            mongo::Status const status = dbclientTemp->connect(node, "Robomongo");
            if (status.isOK())
            {
                _scriptEngine->init(_isLoadMongoRcJs, node.toString());
                MongoShellExecResult const& result = _scriptEngine->exec("rs.status()", "");
                if (!result.results().empty()) {
                    auto const& resultDocs = result.results().front().documents();
                    if (!resultDocs.empty()) {
                        setName = resultDocs.front()->bsonObj().getStringField("set");
                        if (!setName.empty()) // We get the information, finish the loop
                            break;
                    }
                }
            }
        }

        return setName;
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

    void MongoWorker::pingDatabase(mongo::DBClientBase *dbclient) const
    {
        // Building { ping: 1 }
        mongo::BSONObjBuilder command;
        command.append("ping", 1);
        mongo::BSONObj result;
        std::string authBase = getAuthBase();
        if (authBase.empty()) {
            dbclient->runCommand("admin", command.obj(), result);
        }
        else {
            dbclient->runCommand(authBase, command.obj(), result);
        }
    }
}
