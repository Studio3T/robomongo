#include "robomongo/core/mongodb/MongoWorker.h"

#include <QThread>

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
#include "robomongo/core/utils/Logger.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    MongoWorker::MongoWorker(ConnectionSettings *connection,bool isLoadMongoRcJs, int batchSize, QObject *parent) : QObject(parent),
        _connection(connection),
        _scriptEngine(NULL),
        _dbclient(NULL),
        _isAdmin(true),
        _isLoadMongoRcJs(isLoadMongoRcJs),
        _batchSize(batchSize),
        _timerId(-1)
    {         
        _thread = new QThread(this);
        moveToThread(_thread);
        VERIFY(connect( _thread, SIGNAL(started()), this, SLOT(init()) ));
        _thread->start();
    }

    void MongoWorker::timerEvent(QTimerEvent *event)
    {
        if (_timerId==event->timerId())
        {
            keepAlive();
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
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::init()
    {        
        try {
            _scriptEngine = new ScriptEngine(_connection);
            _scriptEngine->init(_isLoadMongoRcJs);
            _scriptEngine->use(_connection->defaultDatabase());
            _scriptEngine->setBatchSize(_batchSize);
            _timerId = startTimer(pingTimeMs);
        }
        catch (const std::exception &ex) {
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    MongoWorker::~MongoWorker()
    {
        delete _dbclient;
        delete _connection;
        _thread->quit();
        if (!_thread->wait(2000))
            _thread->terminate();

        delete _scriptEngine;
        delete _thread;
    }

    /**
     * @brief Initiate connection to MongoDB
     */
    void MongoWorker::handle(EstablishConnectionRequest *event)
    {
        QMutexLocker lock(&_firstConnectionMutex);

        try {
            mongo::DBClientBase *conn = getConnection();
            bool hasPrimary = _connection->hasEnabledPrimaryCredential();
            if (hasPrimary) {
                std::string errmsg;
                bool ok = conn->auth(
                    _connection->primaryCredential()->databaseName(),
                    _connection->primaryCredential()->userName(),
                    _connection->primaryCredential()->userPassword(), errmsg);

                if (!ok) {
                    throw std::runtime_error("Unable to authorize");
                }

                // If authentication succeed and database name is 'admin' -
                // then user is admin, otherwise user is not admin
                std::string dbName = _connection->primaryCredential()->databaseName();
                std::transform(dbName.begin(), dbName.end(), dbName.begin(), ::tolower);
                if (dbName.compare("admin") != 0) // dbName is NOT "admin"
                    _isAdmin = false;
            }
            boost::scoped_ptr<MongoClient> client(getClient());
            //conn->done();
            std::vector<std::string> dbNames = getDatabaseNamesSafe();
            reply(event->sender(), new EstablishConnectionResponse(this, ConnectionInfo(_connection->getFullAddress(), dbNames, client->getVersion()) ));
        } catch(const std::exception &ex) {
            reply(event->sender(), new EstablishConnectionResponse(this, EventError("Unable to connect to MongoDB")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    std::string MongoWorker::getAuthBase() const
    {
        bool hasPrimary = _connection->hasEnabledPrimaryCredential();
        if(hasPrimary){
            return _connection->primaryCredential()->databaseName();
        }
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
        }
        catch(const std::exception &ex)
        {
            if(!authBase.empty())
                result.push_back(authBase);
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
        return result;
    }

    /**
     * @brief Load list of all database names
     */
    void MongoWorker::handle(LoadDatabaseNamesRequest *event)
    {
        // If user not an admin - he doesn't have access to mongodb 'listDatabases' command
        // Non admin user has access only to the single database he specified while performing auth.
        std::vector<std::string> dbNames = getDatabaseNamesSafe();
            
        if(dbNames.size()){
            reply(event->sender(), new LoadDatabaseNamesResponse(this, dbNames));
        }else{
            reply(event->sender(), new LoadDatabaseNamesResponse(this, EventError("Unable to load database names.")));
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
            reply(event->sender(), new LoadCollectionNamesResponse(this, EventError("Unable to load list of collections.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
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
            reply(event->sender(), new LoadUsersResponse(this, EventError("Unable to load list of users.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
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
            reply(event->sender(), new LoadCollectionIndexesResponse(this, std::vector<EnsureIndexInfo>()));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(EnsureIndexRequest *event)
    {
        const EnsureIndexInfo &newInfo = event->newInfo();
        const EnsureIndexInfo &oldInfo = event->oldInfo();
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->ensureIndex(oldInfo,newInfo);
            const std::vector<EnsureIndexInfo> &ind = client->getIndexes(newInfo._collection);
            client->done();

            reply(event->sender(), new LoadCollectionIndexesResponse(this, ind));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new LoadCollectionIndexesResponse(this, std::vector<EnsureIndexInfo>()));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(DropCollectionIndexRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->dropIndexFromCollection(event->collection(),event->name());
            client->done();
            reply(event->sender(), new DeleteCollectionIndexResponse(this, event->collection(), event->name()));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new DeleteCollectionIndexResponse(this, event->collection(), std::string() ));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }            
    }

    void MongoWorker::handle(EditIndexRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->renameIndexFromCollection(event->collection(),event->oldIndex(),event->newIndex());
            const std::vector<EnsureIndexInfo> &ind = client->getIndexes(event->collection());
            client->done();

            reply(event->sender(), new LoadCollectionIndexesResponse(this, ind));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new LoadCollectionIndexesResponse(this, std::vector<EnsureIndexInfo>()));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
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
            reply(event->sender(), new LoadFunctionsResponse(this, EventError("Unable to load list of functions.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
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
            reply(event->sender(), new InsertDocumentResponse(this, EventError("Unable to insert document.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
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
            reply(event->sender(), new RemoveDocumentResponse(this, EventError("Unable to remove documents.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
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
            reply(event->sender(), new ExecuteQueryResponse(this, EventError("Unable to complete query.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    /**
     * @brief Execute javascript
     */
    void MongoWorker::handle(ExecuteScriptRequest *event)
    {
        try {
            MongoShellExecResult result = _scriptEngine->exec(event->script, event->databaseName);
            reply(event->sender(), new ExecuteScriptResponse(this, result, event->script.empty()));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new ExecuteScriptResponse(this, EventError("Unable to complete query.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(AutocompleteRequest *event)
    {
        try {
            QStringList list = _scriptEngine->complete(event->prefix, event->mode);
            reply(event->sender(), new AutocompleteResponse(this, list, event->prefix));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new ExecuteScriptResponse(this, EventError("Unable to autocomplete query.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(CreateDatabaseRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->createDatabase(event->database());
            client->done();

            reply(event->sender(), new CreateDatabaseResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new CreateDatabaseResponse(this, EventError("Unable to create database.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(DropDatabaseRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->dropDatabase(event->database());
            client->done();

            reply(event->sender(), new DropDatabaseResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new DropDatabaseResponse(this, EventError("Unable to drop database.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(CreateCollectionRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            client->createCollection(event->ns());
            client->done();

            reply(event->sender(), new CreateCollectionResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new CreateCollectionResponse(this, EventError("Unable to create collection.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
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
            reply(event->sender(), new DropCollectionResponse(this, EventError("Unable to drop collection.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
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
            reply(event->sender(), new RenameCollectionResponse(this, EventError("Unable to rename collection.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
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
            reply(event->sender(), new DuplicateCollectionResponse(this, EventError("Unable to duplicate collection.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(CopyCollectionToDiffServerRequest *event)
    {
        try {
            boost::scoped_ptr<MongoClient> client(getClient());
            MongoWorker *cl = event->worker();
            client->copyCollectionToDiffServer(cl->_dbclient,event->from(),event->to());
            client->done();

            reply(event->sender(), new CopyCollectionToDiffServerResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new CopyCollectionToDiffServerResponse(this, EventError("Unable to copy collection.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
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
            reply(event->sender(), new CreateUserResponse(this, EventError("Unable to create/ovewrite user.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
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
            reply(event->sender(), new DropUserResponse(this, EventError("Unable to drop user.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
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
            reply(event->sender(), new CreateFunctionResponse(this, EventError("Unable to create/ovewrite function.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
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
            reply(event->sender(), new DropFunctionResponse(this, EventError("Unable to drop function.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    mongo::DBClientConnection *MongoWorker::getConnection()
    {
        if (!_dbclient) {
            mongo::DBClientConnection *conn = new mongo::DBClientConnection(true);
            conn->connect(_connection->info());
            _dbclient = conn;
        }
        return _dbclient;
    }

    MongoClient *MongoWorker::getClient()
    {
        return new MongoClient(getConnection());
    }

    /**
     * @brief Send event to this MongoWorker
     */
    void MongoWorker::send(Event *event)
    {
        AppRegistry::instance().bus()->send(this, event);
    }

    /**
     * @brief Send reply event to object 'receiver'
     */
    void MongoWorker::reply(QObject *receiver, Event *event)
    {
        AppRegistry::instance().bus()->send(receiver, event);
    }
}
