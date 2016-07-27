#include "robomongo/core/mongodb/MongoWorker.h"

#include <QThread>

#include <mongo/util/net/ssl_manager.h>
#include <mongo/util/net/ssl_options.h>

#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/engine/ScriptEngine.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/mongodb/MongoClient.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/domain/MongoCollectionInfo.h"
#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/settings/SslSettings.h"
#include "robomongo/core/utils/Logger.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    MongoWorker::MongoWorker(ConnectionSettings *connection, bool isLoadMongoRcJs, int batchSize,
                             int mongoTimeoutSec, int shellTimeoutSec, QObject *parent) : QObject(parent),
        _connection(connection),
        _scriptEngine(NULL),
        _dbclient(nullptr),
        _isAdmin(true),
        _isLoadMongoRcJs(isLoadMongoRcJs),
        _batchSize(batchSize),
        _timerId(-1),
        _dbAutocompleteCacheTimerId(-1),
        _mongoTimeoutSec(mongoTimeoutSec),
        _shellTimeoutSec(shellTimeoutSec),
        _isQuiting(0)
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

        if (_dbAutocompleteCacheTimerId == event->timerId() &&
            _scriptEngine != NULL) {
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
        try {
            _scriptEngine = new ScriptEngine(_connection, _shellTimeoutSec);
            _scriptEngine->init(_isLoadMongoRcJs);
            _scriptEngine->use(_connection->defaultDatabase());
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

        delete _connection;
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

        try {
            mongo::DBClientBase *conn = getConnection(true);
            if (conn == NULL) {
                auto errorReason = std::string("Connection failure: Unknown error.");
                if (_connection->sslSettings()->sslEnabled())
                {
                    // Note: Currently mongo-shell does not provide any interface to fetch actual error details
                    // for some SSL connection failures that's why we are unable to show exact error here. 
                    errorReason = "SSL tunnel failure: Network is unreachable or SSL connection rejected by server.";
                }
                else
                {
                    errorReason = "Network is unreachable.";
                }

                reply(event->sender(), new EstablishConnectionResponse(this,
                    EventError(errorReason), event->connectionType,
                    EstablishConnectionResponse::MongoConnection));
                return;
            }

            if (_connection->hasEnabledPrimaryCredential()) {
                CredentialSettings *credentials = _connection->primaryCredential();

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

            init();

            reply(event->sender(), new EstablishConnectionResponse(this, ConnectionInfo(_connection->getFullAddress(), 
                dbNames, client->getVersion(), client->getStorageEngineType()), event->connectionType));
        } catch(const std::exception &ex) {
            auto errorReason = _connection->sslSettings()->sslEnabled() ?
                EstablishConnectionResponse::ErrorReason::MongoSslConnection : 
                EstablishConnectionResponse::ErrorReason::MongoAuth;
            reply(event->sender(), 
                new EstablishConnectionResponse(this, EventError(ex.what()), event->connectionType, errorReason));
        }
    }

    std::string MongoWorker::getAuthBase() const
    {
        if (_connection->hasEnabledPrimaryCredential())
            return _connection->primaryCredential()->databaseName();

        return std::string();
    }

    MongoWorker::DatabasesContainerType MongoWorker::getDatabaseNamesSafe()
    {        
        DatabasesContainerType result;
        std::string authBase = getAuthBase();
        if (!_isAdmin && !authBase.empty()) {
            result.push_back(_connection->primaryCredential()->databaseName());
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

            MongoShellExecResult result = _scriptEngine->exec(event->script, event->databaseName);
            reply(event->sender(), new ExecuteScriptResponse(this, result, event->script.empty()));
        } catch(const mongo::DBException &ex) {
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
        if (!_dbclient) {
            // Timeout for operations
            // Connect timeout is fixed, but short, at 5 seconds (see headers for DBClientConnection)
            _dbclient = std::unique_ptr<mongo::DBClientConnection>(new mongo::DBClientConnection(true, _mongoTimeoutSec));

            // Update global mongo SSL settings according to SSL enable/disabled status
            if (_connection->sslSettings()->sslEnabled())
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

            mongo::Status status = _dbclient->connect(_connection->info());

            if (!status.isOK() && mayReturnNull) {
                return nullptr;
            }
        }
        return _dbclient.get();
    }

    MongoClient *MongoWorker::getClient()
    {
        return new MongoClient(getConnection());
    }

    void MongoWorker::updateGlobalSSLparams() const
    {
        resetGlobalSSLparams();
        const SslSettings * const sslSettings = _connection->sslSettings();
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
        mongo::sslGlobalParams.sslAllowInvalidCertificates = "";
        mongo::sslGlobalParams.sslCAFile = "";
        mongo::sslGlobalParams.sslPEMKeyFile = "";
        mongo::sslGlobalParams.sslPEMKeyPassword = "";
        mongo::sslGlobalParams.sslCRLFile = "";
        mongo::sslGlobalParams.sslAllowInvalidHostnames = false;
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
