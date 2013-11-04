#include "robomongo/core/mongodb/MongoWorker.h"

#include <QThread>

#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/engine/ScriptEngine.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/mongodb/MongoClient.h"
#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/domain/MongoCollectionInfo.h"
#include "robomongo/core/utils/Logger.h"
#include "robomongo/core/utils/QtUtils.h"
#include <mongo/client/dbclient_rs.h>

namespace Robomongo
{
    MongoWorker::MongoWorker(IConnectionSettingsBase *connection,bool isLoadMongoRcJs, int batchSize, QObject *parent) : QObject(parent),
        _connection(connection),
        _scriptEngine(NULL),
        _dbclient(NULL),
        _isConnected(false),
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
        ReplicasetConnectionSettings *repSettings = dynamic_cast<ReplicasetConnectionSettings *>(_connection);
        if(repSettings){
            mongo::ReplicaSetMonitor::remove(repSettings->replicaName());// fix crash on exit
        }
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
            CredentialSettings primCred = _connection->primaryCredential();
            if (primCred.isValidAnEnabled()) {
                std::string errmsg;
                CredentialSettings::CredentialInfo inf = primCred.info();
                bool ok = conn->auth(inf._databaseName, inf._userName, inf._userPassword, errmsg);

                if (!ok) {
                    throw std::runtime_error("Unable to authorize");
                }

                // If authentication succeed and database name is 'admin' -
                // then user is admin, otherwise user is not admin
                std::string dbName = inf._databaseName;
                std::transform(dbName.begin(), dbName.end(), dbName.begin(), ::tolower);
                if (dbName.compare("admin") != 0) // dbName is NOT "admin"
                    _isAdmin = false;
            }
            std::vector<std::string> dbNames = getDatabaseNamesSafe();
            reply(event->sender(), new EstablishConnectionResponse(this, ConnectionInfo(_connection->getFullAddress(), dbNames, MongoClient::getVersion(conn)) ));
        } catch(const std::exception &ex) {
            reply(event->sender(), new EstablishConnectionResponse(this, EventError("Unable to connect to MongoDB")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    std::string MongoWorker::getAuthBase() const
    {
        CredentialSettings primCred = _connection->primaryCredential();
        if (primCred.isValidAnEnabled()) {
            CredentialSettings::CredentialInfo inf = primCred.info();
            return inf._databaseName;
        }
        return std::string();
    }

    MongoWorker::DatabasesContainerType MongoWorker::getDatabaseNamesSafe()
    {        
        DatabasesContainerType result;
        std::string authBase = getAuthBase();
        if (!_isAdmin && !authBase.empty()) {
            result.push_back(authBase);
            return result;
        }

        try {
            mongo::DBClientBase *con = getConnection();
            result = MongoClient::getDatabaseNames(con);
        }
        catch(const std::exception &ex)
        {
            if(!authBase.empty())
                result.push_back(authBase);
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }

        std::string defDataBase = _connection->defaultDatabase();
        float version = MongoClient::getVersion(_dbclient);
        if(!defDataBase.empty() && MongoUser::minimumSupportedVersion <= version){
            bool foundDefaultDatabase = false;
            for (DatabasesContainerType::const_iterator it = result.begin(); it != result.end();++it)
            {
                std::string databaseName = *it;
                if (databaseName == defDataBase)
                {
                    foundDefaultDatabase = true;
                    break;
                }
            }
            if (!foundDefaultDatabase){
                result.push_back(defDataBase);
            }
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
            mongo::DBClientBase *con = getConnection();

            std::vector<std::string> stringList = MongoClient::getCollectionNames(con, event->databaseName());
            const std::vector<MongoCollectionInfo> &infos = MongoClient::runCollStatsCommand(con, stringList);

            reply(event->sender(), new LoadCollectionNamesResponse(this, event->databaseName(), infos));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new LoadCollectionNamesResponse(this, EventError("Unable to load list of collections.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(LoadUsersRequest *event)
    {
        try {
            mongo::DBClientBase *con = getConnection();
            const std::vector<MongoUser> &users = MongoClient::getUsers(con, event->databaseName());

            reply(event->sender(), new LoadUsersResponse(this, event->databaseName(), users));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new LoadUsersResponse(this, EventError("Unable to load list of users.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(LoadCollectionIndexesRequest *event)
    {
        try {
            mongo::DBClientBase *con = getConnection();
            const std::vector<EnsureIndexInfo> &ind = MongoClient::getIndexes(con, event->collection());

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
            mongo::DBClientBase *con = getConnection();
            MongoClient::ensureIndex(con, oldInfo,newInfo);
            const std::vector<EnsureIndexInfo> &ind = MongoClient::getIndexes(con, newInfo._collection);

            reply(event->sender(), new LoadCollectionIndexesResponse(this, ind));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new LoadCollectionIndexesResponse(this, std::vector<EnsureIndexInfo>()));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(DropCollectionIndexRequest *event)
    {
        try {
            mongo::DBClientBase *con = getConnection();
            MongoClient::dropIndexFromCollection(con, event->collection(),event->name());
            reply(event->sender(), new DeleteCollectionIndexResponse(this, event->collection(), event->name()));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new DeleteCollectionIndexResponse(this, event->collection(), std::string() ));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }            
    }

    void MongoWorker::handle(EditIndexRequest *event)
    {
        try {
            mongo::DBClientBase *con = getConnection();
            MongoClient::renameIndexFromCollection(con, event->collection(),event->oldIndex(),event->newIndex());
            const std::vector<EnsureIndexInfo> &ind = MongoClient::getIndexes(con, event->collection());

            reply(event->sender(), new LoadCollectionIndexesResponse(this, ind));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new LoadCollectionIndexesResponse(this, std::vector<EnsureIndexInfo>()));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        } 
    }

    void MongoWorker::handle(LoadFunctionsRequest *event)
    {
        try {
            mongo::DBClientBase *con = getConnection();
            const std::vector<MongoFunction> &funs = MongoClient::getFunctions(con, event->databaseName());

            reply(event->sender(), new LoadFunctionsResponse(this, event->databaseName(), funs));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new LoadFunctionsResponse(this, EventError("Unable to load list of functions.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    bool MongoWorker::insertDocument(const mongo::BSONObj &obj, const MongoNamespace &ns, bool overwrite)
    {
        bool result = false;
        if(_dbclient && _isConnected){
            if (overwrite)
                result = MongoClient::saveDocument(_dbclient, obj, ns);
            else
                result = MongoClient::insertDocument(_dbclient, obj, ns);
        }

        return result;
    }

    void MongoWorker::handle(InsertDocumentRequest *event)
    {
        try {
            mongo::DBClientBase *con = getConnection();
            insertDocument(event->obj(), event->ns(), event->overwrite() );
            reply(event->sender(), new InsertDocumentResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new InsertDocumentResponse(this, EventError("Unable to insert document.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(RemoveDocumentRequest *event)
    {
        try {
            mongo::DBClientBase *con = getConnection();
            MongoClient::removeDocuments(con, event->ns(), event->query(), event->justOne());

            reply(event->sender(), new RemoveDocumentResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new RemoveDocumentResponse(this, EventError("Unable to remove documents.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(ExecuteQueryRequest *event)
    {
        try {
            mongo::DBClientBase *con = getConnection();
            std::vector<MongoDocumentPtr> docs = MongoClient::query(con, event->queryInfo());

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
            QStringList list = _scriptEngine->complete(event->prefix);
            reply(event->sender(), new AutocompleteResponse(this, list, event->prefix));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new ExecuteScriptResponse(this, EventError("Unable to autocomplete query.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(CreateDatabaseRequest *event)
    {
        try {
            mongo::DBClientBase *con = getConnection();
            MongoClient::createDatabase(con, event->database());

            reply(event->sender(), new CreateDatabaseResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new CreateDatabaseResponse(this, EventError("Unable to create database.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(DropDatabaseRequest *event)
    {
        try {
            mongo::DBClientBase *con = getConnection();
            MongoClient::dropDatabase(con, event->database());

            reply(event->sender(), new DropDatabaseResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new DropDatabaseResponse(this, EventError("Unable to drop database.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(CreateCollectionRequest *event)
    {
        try {
            mongo::DBClientBase *con = getConnection();
            MongoClient::createCollection(con, event->ns());

            reply(event->sender(), new CreateCollectionResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new CreateCollectionResponse(this, EventError("Unable to create collection.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(DropCollectionRequest *event)
    {
        try {
            mongo::DBClientBase *con = getConnection();
            MongoClient::dropCollection(con, event->ns());

            reply(event->sender(), new DropCollectionResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new DropCollectionResponse(this, EventError("Unable to drop collection.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(RenameCollectionRequest *event)
    {
        try {
            mongo::DBClientBase *con = getConnection();
            MongoClient::renameCollection(con, event->ns(), event->newCollection());

            reply(event->sender(), new RenameCollectionResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new RenameCollectionResponse(this, EventError("Unable to rename collection.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(DuplicateCollectionRequest *event)
    {
        try {
            mongo::DBClientBase *con = getConnection();
            MongoClient::duplicateCollection(con, event->ns(), event->newCollection());

            reply(event->sender(), new DuplicateCollectionResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new DuplicateCollectionResponse(this, EventError("Unable to duplicate collection.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(CopyCollectionToDiffServerRequest *event)
    {
        try {
            mongo::DBClientBase *con = getConnection();
            MongoWorker *cl = event->worker();
            MongoClient::copyCollectionToDiffServer(con, cl->_dbclient,event->from(),event->to());

            reply(event->sender(), new CopyCollectionToDiffServerResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new CopyCollectionToDiffServerResponse(this, EventError("Unable to copy collection.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(CreateUserRequest *event)
    {
        try {
            mongo::DBClientBase *con = getConnection();
            MongoClient::createUser(con, event->database(), event->user(), event->overwrite());

            reply(event->sender(), new CreateUserResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new CreateUserResponse(this, EventError("Unable to create/ovewrite user.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(DropUserRequest *event)
    {
        try {
            mongo::DBClientBase *con = getConnection();
            MongoClient::dropUser(con, event->database(), event->id());

            reply(event->sender(), new DropUserResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new DropUserResponse(this, EventError("Unable to drop user.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(CreateFunctionRequest *event)
    {
        try {
            mongo::DBClientBase *con = getConnection();
            MongoClient::createFunction(con, event->database(), event->function(), event->existingFunctionName());

            reply(event->sender(), new CreateFunctionResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new CreateFunctionResponse(this, EventError("Unable to create/ovewrite function.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    void MongoWorker::handle(DropFunctionRequest *event)
    {
        try {
            mongo::DBClientBase *con = getConnection();
            MongoClient::dropFunction(con, event->database(), event->name());

            reply(event->sender(), new DropFunctionResponse(this));
        } catch(const mongo::DBException &ex) {
            reply(event->sender(), new DropFunctionResponse(this, EventError("Unable to drop function.")));
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
    }

    mongo::DBClientBase *MongoWorker::getConnection()
    {
        IConnectionSettingsBase::ConnectionType conType = _connection->connectionType();
        if (!_dbclient) {            
            if(conType == IConnectionSettingsBase::DIRECT){
                mongo::DBClientConnection *conn = new mongo::DBClientConnection(true);
                _dbclient = conn;
            }
            else if(conType == IConnectionSettingsBase::REPLICASET){
                ReplicasetConnectionSettings *set = dynamic_cast<ReplicasetConnectionSettings *>(_connection);
                VERIFY(set);

                mongo::DBClientReplicaSet *conn = new mongo::DBClientReplicaSet(set->replicaName(),set->serversHostsInfo());           
                _dbclient = conn;
            }
        }

        if(_dbclient && !_isConnected){ //try to connect
            if(conType == IConnectionSettingsBase::DIRECT){
                ConnectionSettings *con = dynamic_cast<ConnectionSettings *>(_connection);
                VERIFY(con);

                mongo::DBClientConnection *conCon = dynamic_cast<mongo::DBClientConnection *>(_dbclient);
                VERIFY(conCon);

                std::string err;
                _isConnected = conCon->connect(con->info(), err);
            }
            else if(conType == IConnectionSettingsBase::REPLICASET){
                mongo::DBClientReplicaSet *setCon = dynamic_cast<mongo::DBClientReplicaSet *>(_dbclient);
                VERIFY(setCon);

                _isConnected = setCon->connect();
            }
        }

        if(!_isConnected){
            throw std::runtime_error("Unable to connect");
        }
        return _dbclient;
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
