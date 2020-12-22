#include "robomongo/core/mongodb/MongoWorker.h"

#include <algorithm>
#include <exception>

#include <QThread>

#include <mongo/client/global_conn_pool.h>
#include <mongo/client/replica_set_monitor.h>
#include <mongo/util/net/ssl_manager.h>
#include <mongo/util/net/ssl_options.h>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/domain/MongoCollectionInfo.h"
#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/engine/ScriptEngine.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/mongodb/MongoClient.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/ReplicaSetSettings.h"
#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/settings/SslSettings.h"
#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/core/utils/Logger.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/utils/StringOperations.h"

namespace Robomongo
{
    std::string const APP_VERSION = PROJECT_VERSION;
    std::string const APP_NAME_VERSION { "robo3t-" + APP_VERSION };

    MongoWorker::MongoWorker(ConnectionSettings *connection, bool isLoadMongoRcJs, int batchSize,
                             double mongoTimeoutSec, int shellTimeoutSec, QObject *parent) 
        : QObject(parent),
        _scriptEngine(nullptr),
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
        // Whitespace removed from the start and the end of host string
        _connSettings->setServerHost(QString::fromStdString(_connSettings->serverHost()).trimmed().toStdString());
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

    void MongoWorker::restartReplicaSetConnection()
    {
        if (!_connSettings->hasEnabledPrimaryCredential())
            return;

        CredentialSettings *credentials = _connSettings->primaryCredential();
        mongo::BSONObj authParams {
            mongo::BSONObjBuilder()
            .append("user", credentials->userName())
            .append("db", credentials->databaseName())
            .append("pwd", credentials->userPassword())
            .append("mechanism", credentials->mechanism())
            .obj()
        };

        _dbclientRepSet.release();
        if(mongo::DBClientBase *conn = getConnection(true).first)
            conn->auth(authParams);
    }

    void MongoWorker::keepAlive()
    {
        try {
            if (_dbclient)
                pingDatabase(_dbclient.get());

            if (_dbclientRepSet)
                pingDatabase(_dbclientRepSet.get());

            if (_scriptEngine)
                _scriptEngine->ping();

        } 
        catch(std::exception &ex) {
            sendLog(this, LogEvent::RBM_WARN, 
                "Failed to ping the server. " + std::string(ex.what()));
        }
    }

    void MongoWorker::init()
    {        
        try {
            _scriptEngine.reset(new ScriptEngine(_connSettings, _shellTimeoutSec));
            _scriptEngine->init(_isLoadMongoRcJs);
            _scriptEngine->use(_connSettings->defaultDatabase());
            _scriptEngine->setBatchSize(_batchSize);
            constexpr int PING_INTERVAL_MSEC { 60 * 1000 };  // 60 seconds
            _timerId = startTimer(PING_INTERVAL_MSEC);
            _dbAutocompleteCacheTimerId = startTimer(30000);
        } catch (const std::exception &ex) {
            auto const msg { "Failed to initialize MongoWorker. Reason: "};
            sendLog(this, LogEvent::RBM_ERROR, msg + std::string(ex.what()));
            throw std::runtime_error(msg + std::string(ex.what()));
        }
    }

    void MongoWorker::interrupt() {
        try {
            if (_isQuiting || !_scriptEngine)
                return;

            _scriptEngine->interrupt();
        } catch(const std::exception &ex) {
            sendLog(this, LogEvent::RBM_ERROR, std::string(ex.what()));
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
            auto const& connAndErrorStr = getConnection(true);
            mongo::DBClientBase *conn = connAndErrorStr.first;           
            
            // --- Connection failed for single server & replica set (no member of the set is reachable)
            if (!conn) 
            {
                auto errorReason = std::string("Connection failure: Unknown error.");
                auto const& connErrorStr = connAndErrorStr.second;

                if (_connSettings->sslSettings()->sslEnabled())
                     errorReason = 
                         "TLS tunnel failure: Network is unreachable or TLS connection rejected by server." + 
                          (connErrorStr.empty() ? "" : " Reason: " + connErrorStr);
                else {  // Non-TLS connections
                    if (_connSettings->isReplicaSet()) {
                        errorReason = "No member of the set is reachable." + 
                                      (connErrorStr.empty() ? "" : " Reason: " + connErrorStr);
                        std::vector<std::pair<std::string, bool>> membersAndHealths;
                        for (auto const& member : _connSettings->replicaSetSettings()->members())
                            membersAndHealths.push_back({ member, false });
                        
                        repSetInfo.reset(new ReplicaSet("", mongo::HostAndPort(), membersAndHealths, errorReason));
                    }
                    else    // single server
                        errorReason = "Network is unreachable." + (connErrorStr.empty() ? "" : " Reason: " + connErrorStr);
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
                ReplicaSet const& setInfo = getReplicaSetInfo();

                // Check if same set name used with different members which is not supported
                auto const& members = _connSettings->replicaSetSettings()->members();
                if (std::find(members.cbegin(), members.cend(), setInfo.primary.toString()) == members.cend()) 
                {   // primary not found between user entered members
                    std::string const errorStr { 
                        "Different members found under same replica set name \"" +
                         setInfo.setName + "\"." 
                    };
                    repSetInfo.reset(new ReplicaSet(setInfo));
                    errorCode = EventError::ErrorCode::ServerHasDifferentMembers;
                    sendLog(this, LogEvent::RBM_ERROR, errorStr);
                    throw std::runtime_error(errorStr);
                }

                if (setInfo.primary.empty()) {  // No reachable primary
                    // Pass possible reachable secondary(ies) info 
                    repSetInfo.reset(new ReplicaSet(setInfo)); 
                    sendLog(this, LogEvent::RBM_ERROR, setInfo.errorStr);
                    throw std::runtime_error(setInfo.errorStr);
                }
                else {  // Primary is reachable, save setInfo and continue
                    repSetInfo.reset(new ReplicaSet(setInfo));
                }
            }

            if (_connSettings->hasEnabledPrimaryCredential()) {
                CredentialSettings const * const credentials = _connSettings->primaryCredential();

                // Building BSON object:
                mongo::BSONObj const authParams { 
                    mongo::BSONObjBuilder()
                    .append("user", credentials->userName())
                    .append("db", credentials->databaseName())
                    .append("pwd", credentials->userPassword())
                    .append("mechanism", credentials->mechanism())
                    .obj()
                };

                conn->auth(authParams);
            }

            boost::scoped_ptr<MongoClient> client(getClient());
            std::vector<std::string> const dbNames = getDatabaseNamesSafe(event);

            // If we do not have databases, it means that we are unable to
            // execute "listdatabases" command and we have nothing to show.
            if (dbNames.size() == 0)
                throw std::runtime_error("Failed to execute \"listdatabases\" command.");

            if (!_connSettings->isReplicaSet())
                init(); // Init MongoWorker for single server (for replica set connections early init is used)

            resetGlobalSSLparams();

            auto connInfo = ConnectionInfo(_connSettings->getFullAddress(), dbNames, client->getVersion(), 
                                           client->dbVersionStr(), client->getStorageEngineType(), event->uuid);

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
            sendLog(this, LogEvent::RBM_ERROR, ex.what());  // todo: duplicate logging?
        }

        return false;
    }

    void MongoWorker::handle(RefreshReplicaSetFolderRequest *event)
    {
		configureSSL();

        try {
            ReplicaSet const& replicaSetInfo = getReplicaSetInfo();
            // Primary is unreachable, but there might be reachable secondary(ies)
            if (replicaSetInfo.primary.empty()) {
                reply(
                    event->sender(), 
                    new RefreshReplicaSetFolderResponse(
                        this, replicaSetInfo, event->expanded, EventError(replicaSetInfo.errorStr)
                    )
                );
                sendLog(this, LogEvent::RBM_ERROR, replicaSetInfo.errorStr);
                return;
            }
            else { // Primary is reachable
                reply(event->sender(), 
                    new RefreshReplicaSetFolderResponse(this, replicaSetInfo, event->expanded));
            }
        }
        catch (const std::exception &ex) {
            reply(
                event->sender(),
                new RefreshReplicaSetFolderResponse(
                    this, ReplicaSet(), event->expanded, EventError(ex.what())
                )
            );
            sendLog(this, LogEvent::RBM_ERROR, ex.what());
        }
    }

    std::string MongoWorker::getAuthBase() const
    {
        if (_connSettings->hasEnabledPrimaryCredential())
            return _connSettings->primaryCredential()->databaseName();

        return std::string();
    }

    std::vector<std::string> MongoWorker::getDatabaseNamesSafe(EstablishConnectionRequest* event /*= nullptr*/)
    {
        std::set<std::string> dbNames;
        auto const primaryCredential { _connSettings->primaryCredential() };

        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            std::vector<std::string> dbNamesFetched { client->getDatabaseNames() };
            dbNames = std::set<std::string> { dbNamesFetched.cbegin(), dbNamesFetched.cend() };
        } 
        catch(const std::exception &ex) {
#if defined(__clang__) 
#pragma clang diagnostic push    
#pragma clang diagnostic ignored "-Wlogical-op-parentheses"
#endif
            bool const informUser {
                event != nullptr &&
                event->connectionType == ConnectionType::ConnectionPrimary &&
                _connSettings->credentialCount() > 0 &&
                !primaryCredential->useManuallyVisibleDbs() ||
                primaryCredential->manuallyVisibleDbs().empty()
            };
#if defined(__clang__) 
#pragma clang diagnostic pop
#endif
            std::string const hint {
                "\n\nHint: If this user has access to a specific database, "
                "please use \"Manually specify visible databases\" option in "
                "Connection Settings window -> Authentication tab."
            };
            sendLog(this, LogEvent::RBM_WARN, ex.what() + hint, informUser);
        }

        if (_connSettings->credentialCount() > 0 &&
            primaryCredential->useManuallyVisibleDbs() &&
            !primaryCredential->manuallyVisibleDbs().empty()
        )
        {        
            QString const manuallyVisibleDbs {
                QString::fromStdString(primaryCredential->manuallyVisibleDbs())
            };

            for (auto const& db : manuallyVisibleDbs.split(',').toStdList())
                dbNames.insert(db.toStdString());
        }

        std::string const authBase = getAuthBase();
        if (!authBase.empty())
            dbNames.insert(authBase);

        return std::vector<std::string> { dbNames.cbegin(), dbNames.cend() };
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
                auto const errorStr{ "Failed to execute \"listdatabases\" command." };
                reply(event->sender(), new LoadDatabaseNamesResponse(this, EventError(errorStr)));
            }
        } catch(const std::exception &ex) {
            reply(event->sender(), new LoadDatabaseNamesResponse(this, EventError(ex.what())));
            sendLog(this, LogEvent::RBM_ERROR, ex.what());
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
        } catch(const std::exception &ex) {
            reply(event->sender(), new LoadCollectionNamesResponse(this, EventError(ex.what())));
            // Logging handled in main thread
        }
    }

    void MongoWorker::handle(LoadUsersRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            const std::vector<MongoUser> &users = client->getUsers(event->databaseName());
            client->done();

            reply(event->sender(), new LoadUsersResponse(this, event->databaseName(), users));
        } catch(const std::exception &ex) {
            reply(event->sender(), new LoadUsersResponse(this, EventError(ex.what())));
            // Logging handled in main thread
        }
    }

    void MongoWorker::handle(LoadCollectionIndexesRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            const std::vector<IndexInfo> &ind = client->getIndexes(event->collection());
            client->done();

            reply(event->sender(), new LoadCollectionIndexesResponse(this, ind));
        } catch(const std::exception &ex) {
            reply(event->sender(), new LoadCollectionIndexesResponse(this, EventError(ex.what())));
            sendLog(this, LogEvent::RBM_ERROR, ex.what());
        }
    }

    void MongoWorker::handle(AddEditIndexRequest *event)
    {
        const IndexInfo &newIndex = event->newInfo();
        const IndexInfo &oldIndex = event->oldInfo();
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->addEditIndex(oldIndex, newIndex);
            client->done();
            reply(event->sender(), new AddEditIndexResponse(this, oldIndex, newIndex));

            std::vector<IndexInfo> const &indexes = client->getIndexes(newIndex._collection);
            reply(event->sender(), new LoadCollectionIndexesResponse(this, indexes));
        } catch(const std::exception &ex) {
            reply(event->sender(), 
                new AddEditIndexResponse(this, EventError(ex.what()), oldIndex, newIndex)
            );
            // Logging handled in main thread
        }
    }

    void MongoWorker::handle(DropCollectionIndexRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->dropIndexFromCollection(event->collection(), event->index());
            client->done();
            reply(event->sender(), 
                new DropCollectionIndexResponse(this, event->collection(), event->index()));
        } catch(const std::exception &ex) {
            reply(event->sender(), 
                new DropCollectionIndexResponse(this, EventError(ex.what()), event->index()));
            // Logging handled in main thread
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
        } catch(const std::exception &ex) {
            reply(event->sender(), new LoadFunctionsResponse(this, EventError(ex.what())));
            // Logging handled in main thread
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
        catch(const std::exception &ex) {
            reply(event->sender(), new InsertDocumentResponse(this, EventError(ex.what())));
            sendLog(this, LogEvent::RBM_ERROR, ex.what());
        }
    }

    void MongoWorker::handle(RemoveDocumentRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());

            client->removeDocuments(event->ns(), event->query(), 
                                    event->removeCount() == RemoveDocumentCount::ONE);
            client->done();

            reply(event->sender(), new RemoveDocumentResponse(this, event->removeCount(), event->index()));
        } 
        catch(const std::exception &ex) {
            reply(event->sender(), new RemoveDocumentResponse(this, EventError(ex.what()), 
                event->removeCount(), event->index()));
            // Logging handled in main thread
        }
    }

    void MongoWorker::handle(ExecuteQueryRequest *event)
    {
        auto const executeQuery = [&]() {
            boost::scoped_ptr<MongoClient> client { getClient() };
            std::vector<MongoDocumentPtr> docs = client->query(event->queryInfo());
            client->done();
            reply(event->sender(),
                new ExecuteQueryResponse(this, event->resultIndex(), event->queryInfo(), docs)
            );
        };

        try {
            executeQuery();
        } catch(const std::exception &ex) {
            QString const NTORETURN_ERROR { "unrecognized field: 'ntoreturn'" };
            bool const ntoreturnError { 
                QString(ex.what()).compare(NTORETURN_ERROR, Qt::CaseInsensitive) == 0 
            };
            // If we have this DocumentDB specific error, try again
            if (ntoreturnError && _dbclient) {
                sendLog(this, LogEvent::RBM_ERROR, std::string(ex.what()));
                try {
                    _dbclient->tagAsDocDb(true);
                    executeQuery();
                    return;
                } 
                catch (const std::exception &ex) {
                    sendLog(this, LogEvent::RBM_ERROR, "Re-try: " + std::string(ex.what()));
                }
            }

            reply(event->sender(), new ExecuteQueryResponse(this, EventError(ex.what())));
            sendLog(this, LogEvent::RBM_ERROR, std::string(ex.what()));
        }
    }

    /**
     * @brief Execute javascript
     */
    void MongoWorker::handle(ExecuteScriptRequest *event)
    {        
        try {           
            if(!_scriptEngine ||
               (_connSettings->isReplicaSet() && !_dbclientRepSet)) {
                auto const error{
                    EventError("MongoDB Shell was not initialized or connection failure")
                };
                reply(event->sender(), new ExecuteScriptResponse(this, error));
                return;
            }

            // Try to handle case where new shell (which was opened when server unreachable) 
            // was re-executed
            if (_scriptEngine->failedScope()) {
                try {
                    _scriptEngine->init(_isLoadMongoRcJs);
                }
                catch (std::exception const& ex) {
                    sendLog(this, LogEvent::RBM_ERROR, 
                        captilizeFirstChar(ex.what()) + ", cannot init mongo scope");
                }
            }

            // todo: should we use dbName from event or _connSettings? 
            MongoShellExecResult result {
                _scriptEngine->exec(
                    event->script, _connSettings->defaultDatabase(), event->aggrInfo
                )
            };

            // To fix the problem where 'result' comes with old primary address.
            if (_connSettings->isReplicaSet()) 
                result.setCurrentServer(
                    _dbclientRepSet->getSuspectedPrimaryHostAndPort().toString()
                );

            if (!result.error()) {                
                reply(
                    event->sender(),
                    new ExecuteScriptResponse(this, result, event->script.empty(), 
                        result.timeoutReached()) // todo: rename to shellTimeout...
                );
                return;
            }

            retry(event);
        } 
        catch(const std::exception &ex) {
            auto const error { EventError(ex.what(), EventError::Unknown) };
            reply(event->sender(), new ExecuteScriptResponse(this, error));
            sendLog(this, LogEvent::RBM_ERROR, ex.what());
        }
    }

    void MongoWorker::retry(ExecuteScriptRequest * event)
    {
        mongo::DBClientBase* mongodbClient {
            _dbclient ? _dbclient.get() :
            dynamic_cast<mongo::DBClientBase*>(_dbclientRepSet.get())
        };

        if (!mongodbClient->isStillConnected())
            mongodbClient->checkConnection();

        MongoShellExecResult const result {
            _scriptEngine->exec(event->script, _connSettings->defaultDatabase())
        };
        if (result.error()) {
            auto const error { EventError(result.errorMessage()) };
            reply(event->sender(), new ExecuteScriptResponse(this, error));
        }
        else {
            reply(
                event->sender(),
                new ExecuteScriptResponse(this, result, event->script.empty(),
                    result.timeoutReached())
            );
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
        } catch(const std::exception &ex) {
            sendLog(this, LogEvent::RBM_ERROR, std::string(ex.what()));
        }
    }

    void MongoWorker::handle(AutocompleteRequest *event)
    {
        try {
            if (!_scriptEngine) {
                reply(event->sender(), 
                    new AutocompleteResponse(this, EventError("MongoDB Shell was not initialized")));
                return;
            }

            QStringList list = _scriptEngine->complete(event->prefix, event->mode);
            reply(event->sender(), new AutocompleteResponse(this, list, event->prefix));
        } catch(const std::exception &ex) {
            reply(event->sender(), new AutocompleteResponse(this, EventError(ex.what())));
            sendLog(this, LogEvent::RBM_ERROR, std::string(ex.what()));            
        }
    }

    void MongoWorker::handle(CreateDatabaseRequest *event)
    {
        std::string dbname = event->database();
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->createDatabase(dbname);

            // Insert to list of created database. Read docs for this hashset in the header
            _createdDbs.insert(dbname);

            reply(event->sender(), new CreateDatabaseResponse(this, dbname));
        } catch(const std::exception &ex) {
            reply(event->sender(), new CreateDatabaseResponse(this, dbname, 
                EventError(ex.what()))
            );
            // Logging handled in main thread
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
        catch(const std::exception &ex) {
            reply(event->sender(), 
                new DropDatabaseResponse(this, event->database, EventError(ex.what()))
            );
            // Logging handled in main thread
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
        } catch(const std::exception &ex) {
            reply(event->sender(), 
                new CreateCollectionResponse(this, collection, EventError(ex.what()))
            );
            // Logging handled in main thread
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
        } catch(const std::exception &ex) {
            reply(event->sender(), 
                new DropCollectionResponse(this, collection, EventError(ex.what()))
            );
            // Logging handled in main thread
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
        } catch(const std::exception &ex) {
            reply(event->sender(), new RenameCollectionResponse(this, EventError(ex.what())));
            // Logging handled in main thread
        }
    }

    void MongoWorker::handle(DuplicateCollectionRequest *event)
    {
        std::string const& sourceCollection = event->ns().collectionName();

        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->duplicateCollection(event->ns(), event->newCollection());
            client->done();

            reply(event->sender(), 
                new DuplicateCollectionResponse(this, sourceCollection, event->newCollection())
            );
        }
        catch (const std::exception &ex) {
            reply(event->sender(), 
                new DuplicateCollectionResponse(this, sourceCollection, EventError(ex.what()))
            );
            // Logging handled in main thread
        }
    }
    
    void MongoWorker::handle(CopyCollectionToDiffServerRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            MongoWorker *cl = event->worker();
            client->copyCollectionToDiffServer(
                cl->_dbclient.get(), event->from(), event->to()
            );
            client->done();

            reply(event->sender(), new CopyCollectionToDiffServerResponse(this));
        } catch(const std::exception &ex) {
            reply(event->sender(), 
                new CopyCollectionToDiffServerResponse(this, EventError(ex.what()))
            );
            sendLog(this, LogEvent::RBM_ERROR, std::string(ex.what()));
        }
    }

    void MongoWorker::handle(CreateUserRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->createUser(event->database(), event->user());
            client->done();

            reply(event->sender(), new CreateUserResponse(this, event->user().name()));
        } catch(const std::exception &ex) {
            reply(event->sender(), 
                new CreateUserResponse(this, event->user().name(), EventError(ex.what()))
            );
            // Logging handled in main thread
        }
    }

    void MongoWorker::handle(DropUserRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->dropUser(event->database(), event->username());
            client->done();

            reply(event->sender(), new DropUserResponse(this, event->username()));
        } catch(const std::exception &ex) {
            reply(event->sender(), 
                new DropUserResponse(this, event->username(), EventError(ex.what()))
            );
            // Logging handled in main thread
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
                    throw std::runtime_error(result.errorMessage());
            }
            else {
                boost::scoped_ptr<MongoClient> client(getClient());
                client->createFunction(event->database(), event->function(), event->existingFunctionName());
                client->done();
            }

            reply(event->sender(), new CreateFunctionResponse(this, functionName));
        } catch(const std::exception &ex) {
            reply(event->sender(), 
                new CreateFunctionResponse(this, functionName, EventError(ex.what()))
            );
            // Logging handled in main thread
        }
    }

    void MongoWorker::handle(DropFunctionRequest *event)
    {
        try {
            if (event->dbVersion() >= 3.4) {
                auto const cmd = "db.system.js.remove( { _id : \"" + event->functionName() + "\" } )";
                MongoShellExecResult const& result = _scriptEngine->exec(cmd, event->database());
                if (result.error())
                    throw std::runtime_error(result.errorMessage());
            }
            else {
                boost::scoped_ptr<MongoClient> client(getClient());
                client->dropFunction(event->database(), event->functionName());
                client->done();
            }

            reply(event->sender(), new DropFunctionResponse(this, event->functionName()));
        }
        catch (const std::exception &ex) {
            reply(event->sender(), 
                new DropFunctionResponse(this, event->functionName(), EventError(ex.what()))
            );
            // Logging handled in main thread
        }
    }

    std::pair<mongo::DBClientBase*, std::string> MongoWorker::getConnection(bool mayReturnNull /* = false */)
    {
        configureSSL();

        // --- Perform connection ---
        if (_connSettings->isReplicaSet()) { // connection to replica set 
            if(_dbclientRepSet)
                return { _dbclientRepSet.get(), "" };

            init(); // Init mongoworker for early-use of _scriptEngine

            // Step-1: Use user entered set name or retrieve set name from cache or from a reachable member
            std::string setName = _connSettings->replicaSetSettings()->setNameUserEntered();
            if (setName.empty()) {
                setName = _connSettings->replicaSetSettings()->cachedSetName();
                if (setName.empty()) // If there is no cached set name, get it from an on-line replica node
                    setName = connectAndGetReplicaSetName();

                if (setName.empty())   // It is not possible to continue with empty set name
                    return { nullptr, "It is not possible to continue with empty set name" };
            }

            // Step-2: Try connect to replica set with set name
            auto const& membersHostsAndPorts = _connSettings->replicaSetSettings()->membersToHostAndPort();
            _dbclientRepSet.reset(new mongo::DBClientReplicaSet {
                 setName, membersHostsAndPorts, APP_NAME_VERSION, _mongoTimeoutSec                 
            });
                
            if (!_dbclientRepSet->connect()) 
                return { nullptr, "Connect failed" };
            else 
                return { _dbclientRepSet.get(), "" };
        }
        else {  // connection to single server
            if(_dbclient)
                return { _dbclient.get(), "" };
            
            // Timeout for operations
            // Connect timeout is fixed, but short, at 5 seconds (see headers for DBClientConnection)
            _dbclient.reset(new mongo::DBClientConnection { true, _mongoTimeoutSec });
            mongo::Status const& status = _dbclient->connect(_connSettings->hostAndPort(), APP_NAME_VERSION);
            if (!status.isOK() && mayReturnNull) 
                return { nullptr, status.reason() };
            else 
                return { _dbclient.get(), "" };
        }
    }

    MongoClient *MongoWorker::getClient()
    {
        return new MongoClient(getConnection().first);
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

    // todo: From 1.4, this function started to return incorrect member healths when set is unreachable. 
    //       Needs more investigation.
    ReplicaSet MongoWorker::getReplicaSetInfo() const
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

        setName = repSetMonitor->getName();
        auto const readPrimaryOnly = mongo::ReadPreferenceSetting(mongo::ReadPreference::PrimaryOnly, mongo::TagSet());
        auto const primaryFuture = 
            repSetMonitor->getHostOrRefresh(readPrimaryOnly, mongo::Milliseconds(2000)); 

        auto const& primaryStatus{ primaryFuture.waitNoThrow() };
        if (primaryStatus.isOK())
            primary = primaryFuture.get();

        QStringList servers;
        // i.e. setNameAndMembers: "repset/localhost:27017,localhost:27018,localhost:27019"
        QString const setNameAndMembers = QString::fromStdString(repSetMonitor->getServerAddress());
        QStringList const result = setNameAndMembers.split("/");
        if (result.size() > 1) 
            servers = result[1].split(",");

        // Save address and health of replica members
        for (QString const& server : servers) {
            auto const hostAndPort = mongo::HostAndPort(mongo::StringData(server.toStdString()));
            membersAndHealths.push_back({ server.toStdString(), repSetMonitor->isHostUp(hostAndPort) });
        }

        return ReplicaSet(setName, primary, membersAndHealths, primaryStatus.reason());
    }

    std::string MongoWorker::connectAndGetReplicaSetName() const
    {
        auto const dbclientTemp { std::make_unique<mongo::DBClientConnection>(true, 10) };
        std::string setName = "";

        // Try connecting to the nodes one by one until getting replica set name.
        for (auto const& node : _connSettings->replicaSetSettings()->membersToHostAndPort())
        {
            if (!dbclientTemp->connect(node, APP_NAME_VERSION).isOK())
                return "";

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
        std::string const authBase = getAuthBase();
        dbclient->runCommand(authBase.empty() ? "admin" : authBase, command.obj(), result);
    }
}
