#include "robomongo/core/mongodb/MongoWorker.h"

#include <QThread>
#include <QApplication>

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
#include "robomongo/core/events/MongoEventsGui.hpp"

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

    void MongoWorker::saveObject(const mongo::BSONObj &obj, const MongoNamespace &ns, bool overwrite, ErrorInfo &er)
    {
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try {
                if (overwrite)
                    MongoClient::saveDocument(con, obj, ns);
                else
                    MongoClient::insertDocument(con, obj, ns);
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo(ex.what(), ErrorInfo::_EXCEPTION);
                LOG_MSG(er._description, mongo::LL_ERROR);
            }
        }
    }

    void MongoWorker::customEvent(QEvent *event)
    {
        QEvent::Type type = event->type();
        if (type==static_cast<QEvent::Type>(SaveObjectEvent::EventType)){
            SaveObjectEvent *ev = static_cast<SaveObjectEvent*>(event);
            SaveObjectEvent::value_type v = ev->value();
            ErrorInfo er;
            saveObject(v._obj, v._ns, v._overwrite, er);            
            qApp->postEvent(ev->sender(), new SaveObjectEvent(this, ev->value(), er));
        }
        return BaseClass::customEvent(event);
    }

    void MongoWorker::timerEvent(QTimerEvent *event)
    {
        if (_timerId==event->timerId())
        {
            ErrorInfo er;
            keepAlive(er);
        }
    }

    void MongoWorker::keepAlive(ErrorInfo &er)
    {
        mongo::DBClientBase *con = getConnection(er);

        try {
            if (!er.isError()) {
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
            er = ErrorInfo(ex.what(), ErrorInfo::_EXCEPTION);
            LOG_MSG(er._description, mongo::LL_ERROR);
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
        ErrorInfo er;
        ConnectionInfo inf;
        
        mongo::DBClientBase *con = getConnection(er);
        std::vector<std::string> dbNames;
        float vers = 0.0f;

        if(!er.isError()){
            try {
                CredentialSettings primCred = _connection->primaryCredential();
                if (primCred.isValidAnEnabled()) {
                    std::string errmsg;
                    CredentialSettings::CredentialInfo inf = primCred.info();
                    bool ok = con->auth(inf._databaseName, inf._userName, inf._userPassword, errmsg);

                    if (ok) {
                        // If authentication succeed and database name is 'admin' -
                        // then user is admin, otherwise user is not admin
                        std::string dbName = inf._databaseName;
                        std::transform(dbName.begin(), dbName.end(), dbName.begin(), ::tolower);
                        if (dbName.compare("admin") != 0) // dbName is NOT "admin"
                            _isAdmin = false;                                             
                    }
                    else{
                        er = ErrorInfo("Unable to authorize", ErrorInfo::_ERROR);
                        LOG_MSG(er._description, mongo::LL_ERROR); 
                    }                    
                }
                
                if(!er.isError()){
                    dbNames = getDatabaseNamesSafe(er);
                    vers = MongoClient::getVersion(con);
                }
            } catch(const std::exception &ex) {
                er = ErrorInfo("Unable to connect to MongoDB", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }

        reply(event->sender(), new EstablishConnectionResponse(this, ConnectionInfo(_connection->getFullAddress(), dbNames, vers) , er));
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

    MongoWorker::DatabasesContainerType MongoWorker::getDatabaseNamesSafe(ErrorInfo &er) //needed some refactoring
    {        
        DatabasesContainerType result;
        std::string authBase = getAuthBase();
        if (!_isAdmin && !authBase.empty()) {
            result.push_back(authBase);
            return result;
        }

        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try {           
                result = MongoClient::getDatabaseNames(con);
            }
            catch(const std::exception &ex)
            {
                if(!authBase.empty())
                    result.push_back(authBase);

                er = ErrorInfo(ex.what(), ErrorInfo::_EXCEPTION);
                LOG_MSG(er._description, mongo::LL_ERROR);
            }

            std::string defDataBase = _connection->defaultDatabase();
            float version = MongoClient::getVersion(_dbclient);
            if(!defDataBase.empty() && MongoUser::minimumSupportedVersion <= version){ //only for new versions
                
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
            if(!result.size()){
                er = ErrorInfo("Unable to load database names.", ErrorInfo::_ERROR);
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
        ErrorInfo er;
        std::vector<std::string> dbNames = getDatabaseNamesSafe(er);
        reply(event->sender(), new LoadDatabaseNamesResponse(this, dbNames, er));
    }

    /**
     * @brief Load list of all collection names
     */
    void MongoWorker::handle(LoadCollectionNamesRequest *event)
    {
        ErrorInfo er;
        std::vector<MongoCollectionInfo> infos;
        
        mongo::DBClientBase *con = getConnection(er);

        if (!er.isError()){
            try {
                std::vector<std::string> stringList = MongoClient::getCollectionNames(con, event->databaseName());
                infos = MongoClient::runCollStatsCommand(con, stringList);
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to load list of collections.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }

        reply(event->sender(), new LoadCollectionNamesResponse(this, event->databaseName(), infos, er));
    }

    void MongoWorker::handle(LoadUsersRequest *event)
    {
        ErrorInfo er;
        std::vector<MongoUser> users;
        
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try {            
                users = MongoClient::getUsers(con, event->databaseName());           
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to load list of users.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }      

        reply(event->sender(), new LoadUsersResponse(this, event->databaseName(), users, er));
    }

    void MongoWorker::handle(LoadCollectionIndexesRequest *event)
    {
        ErrorInfo er;
        std::vector<EnsureIndexInfo> ind;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try {
                ind = MongoClient::getIndexes(con, event->collection());
            } catch(const mongo::DBException &ex) {                
                er = ErrorInfo(ex.what(), ErrorInfo::_EXCEPTION);
                LOG_MSG(er._description, mongo::LL_ERROR);
            }
        }
        reply(event->sender(), new LoadCollectionIndexesResponse(this, ind, er));
    }

    void MongoWorker::handle(EnsureIndexRequest *event)
    {
        const EnsureIndexInfo &newInfo = event->newInfo();
        const EnsureIndexInfo &oldInfo = event->oldInfo();
        std::vector<EnsureIndexInfo> ind;
        ErrorInfo er;
        mongo::DBClientBase *con = getConnection(er);

        if (!er.isError()){
            try {                
                MongoClient::ensureIndex(con, oldInfo,newInfo);
                ind = MongoClient::getIndexes(con, newInfo._collection);
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo(ex.what(), ErrorInfo::_EXCEPTION);
                LOG_MSG(er._description, mongo::LL_ERROR);
            }
        }
        reply(event->sender(), new LoadCollectionIndexesResponse(this, ind, er));
    }

    void MongoWorker::handle(DropCollectionIndexRequest *event)
    {
        ErrorInfo er;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try {
                MongoClient::dropIndexFromCollection(con, event->collection(),event->name());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo(ex.what(), ErrorInfo::_EXCEPTION);
                LOG_MSG(er._description, mongo::LL_ERROR);
            } 
        }
        reply(event->sender(), new DropCollectionIndexResponse(this, event->collection(), event->name(), er));
    }

    void MongoWorker::handle(EditIndexRequest *event)
    {
        ErrorInfo er;
        mongo::DBClientBase *con = getConnection(er);
        std::vector<EnsureIndexInfo> ind;
        if (!er.isError()){
            try{
                MongoClient::renameIndexFromCollection(con, event->collection(),event->oldIndex(),event->newIndex());
                ind = MongoClient::getIndexes(con, event->collection());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo(ex.what(), ErrorInfo::_EXCEPTION);
                LOG_MSG(er._description, mongo::LL_ERROR);
            } 
        }
        reply(event->sender(), new LoadCollectionIndexesResponse(this, ind, er));
    }

    void MongoWorker::handle(LoadFunctionsRequest *event)
    {
        ErrorInfo er;
        mongo::DBClientBase *con = getConnection(er);
        std::vector<MongoFunction> funs;
        if (!er.isError()){
            try{
                funs = MongoClient::getFunctions(con, event->databaseName());
            } catch(const mongo::DBException &ex) { 
                er = ErrorInfo("Unable to load list of functions.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
        reply(event->sender(), new LoadFunctionsResponse(this,event->databaseName(), funs, er));
    }

    void MongoWorker::handle(InsertDocumentRequest *event)
    {
        ErrorInfo er;
        saveObject(event->obj(), event->ns(), event->overwrite(), er);
        reply(event->sender(), new InsertDocumentResponse(this,er));
    }

    void MongoWorker::handle(RemoveDocumentRequest *event)
    {
        ErrorInfo er;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                MongoClient::removeDocuments(con, event->ns(), event->query(), event->justOne());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to remove documents.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
        reply(event->sender(), new RemoveDocumentResponse(this, er));
    }

    void MongoWorker::handle(ExecuteQueryRequest *event)
    {
        ErrorInfo er;
        mongo::DBClientBase *con = getConnection(er);
        std::vector<MongoDocumentPtr> docs;
        if (!er.isError()){
            try{
                docs = MongoClient::query(con, event->queryInfo());    
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to complete query.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
        reply(event->sender(), new ExecuteQueryResponse(this, event->resultIndex(), event->queryInfo(), docs, er));
    }

    /**
     * @brief Execute javascript
     */
    void MongoWorker::handle(ExecuteScriptRequest *event)
    {
        ErrorInfo er;
        MongoShellExecResult result;
        try {
            result = _scriptEngine->exec(event->script, event->databaseName);   
        } catch(const mongo::DBException &ex) {
            er = ErrorInfo("Unable to complete query.", ErrorInfo::_EXCEPTION);
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
        reply(event->sender(), new ExecuteScriptResponse(this, result, event->script.empty(), er));
    }

    void MongoWorker::handle(AutocompleteRequest *event)
    {
        ErrorInfo er;
        QStringList list;
        try {
            list = _scriptEngine->complete(event->prefix);            
        } catch(const mongo::DBException &ex) {
            er = ErrorInfo("Unable to autocomplete query.", ErrorInfo::_EXCEPTION);
            LOG_MSG(ex.what(), mongo::LL_ERROR);
        }
        reply(event->sender(), new AutocompleteResponse(this, list, event->prefix, er));
    }

    void MongoWorker::handle(CreateDatabaseRequest *event)
    {
        ErrorInfo er;
        mongo::DBClientBase *con = getConnection(er);
        std::vector<MongoDocumentPtr> docs;
        if (!er.isError()){
            try{
                MongoClient::createDatabase(con, event->database());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to create database.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
        reply(event->sender(), new CreateDatabaseResponse(this, er));
    }

    void MongoWorker::handle(DropDatabaseRequest *event)
    {
        ErrorInfo er;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                MongoClient::dropDatabase(con, event->database());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to drop database.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
        reply(event->sender(), new DropDatabaseResponse(this, er));
    }

    void MongoWorker::handle(CreateCollectionRequest *event)
    {
        ErrorInfo er;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                MongoClient::createCollection(con, event->ns());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to create collection.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
        reply(event->sender(), new CreateCollectionResponse(this, er));
    }

    void MongoWorker::handle(DropCollectionRequest *event)
    {
        ErrorInfo er;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                MongoClient::dropCollection(con, event->ns());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to drop collection.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
        reply(event->sender(), new DropCollectionResponse(this, er));
    }

    void MongoWorker::handle(RenameCollectionRequest *event)
    {
        ErrorInfo er;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                MongoClient::renameCollection(con, event->ns(), event->newCollection());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to rename collection.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
        reply(event->sender(), new RenameCollectionResponse(this, er));
    }

    void MongoWorker::handle(DuplicateCollectionRequest *event)
    {
        ErrorInfo er;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                MongoClient::duplicateCollection(con, event->ns(), event->newCollection());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to duplicate collection.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
        reply(event->sender(), new DuplicateCollectionResponse(this, er));
    }

    void MongoWorker::handle(CopyCollectionToDiffServerRequest *event)
    {
        ErrorInfo er;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                MongoWorker *cl = event->worker();
                MongoClient::copyCollectionToDiffServer(con, cl->_dbclient,event->from(),event->to());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to copy collection.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
        reply(event->sender(), new CopyCollectionToDiffServerResponse(this, er));
    }

    void MongoWorker::handle(CreateUserRequest *event)
    {
        ErrorInfo er;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                MongoClient::createUser(con, event->database(), event->user(), event->overwrite());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to create/ovewrite user.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
        reply(event->sender(), new CreateUserResponse(this, er));
    }

    void MongoWorker::handle(DropUserRequest *event)
    {
        ErrorInfo er;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                MongoClient::dropUser(con, event->database(), event->id());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to drop user.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
        reply(event->sender(), new DropUserResponse(this, er));
    }

    void MongoWorker::handle(CreateFunctionRequest *event)
    {
        ErrorInfo er;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                MongoClient::createFunction(con, event->database(), event->function(), event->existingFunctionName());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to create/ovewrite function.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
        reply(event->sender(), new CreateFunctionResponse(this, er));
    }

    void MongoWorker::handle(DropFunctionRequest *event)
    {
        ErrorInfo er;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                MongoClient::dropFunction(con, event->database(), event->name());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to drop function.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
        reply(event->sender(), new DropFunctionResponse(this, er));
    }

    mongo::DBClientBase *MongoWorker::getConnection(ErrorInfo &er)
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
            er._errorType = ErrorInfo::_ERROR;
            er._description = "Unable to connect";
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
