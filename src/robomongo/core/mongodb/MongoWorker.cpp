#include "robomongo/core/mongodb/MongoWorker.h"

#include <QThread>
#include <QApplication>

#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/domain/MongoCollectionInfo.h"
#include "robomongo/core/events/MongoEvents.hpp"
#include "robomongo/core/engine/ScriptEngine.h"
#include "robomongo/core/utils/Logger.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/core/domain/MongoServer.h"

#include "robomongo/shell/db/json.h"
#include <mongo/client/dbclient_rs.h>

namespace
{
    Robomongo::EnsureIndex makeEnsureIndexInfoFromBsonObj(
        const Robomongo::MongoCollectionInfo &collection,
        const mongo::BSONObj &obj)
    {
        using namespace Robomongo::BsonUtils;
        Robomongo::EnsureIndex info(collection);
        info._name = getField<mongo::String>(obj, "name");
        mongo::BSONObj keyObj = getField<mongo::Object>(obj,"key");
        if (keyObj.isValid()) {
            info._request = jsonString(keyObj, mongo::TenGen, 1, Robomongo::DefaultEncoding, Robomongo::Utc);
        }
        info._unique = getField<mongo::Bool>(obj, "unique");
        info._backGround = getField<mongo::Bool>(obj, "background");
        info._dropDups = getField<mongo::Bool>(obj, "dropDups");
        info._sparse = getField<mongo::Bool>(obj, "sparse");
        info._ttl = obj.getIntField("expireAfterSeconds");
        info._defaultLanguage = getField<mongo::String>(obj, "default_language");
        info._languageOverride = getField<mongo::String>(obj, "language_override");
        mongo::BSONObj weightsObj = getField<mongo::Object>(obj, "weights");
        if (weightsObj.isValid()) {
            info._textWeights = jsonString(weightsObj, mongo::TenGen, 1, Robomongo::DefaultEncoding, Robomongo::Utc);
        }
        return info;
    }
}

namespace Robomongo
{
    MongoWorker::MongoWorker(IConnectionSettingsBase *connection, bool isLoadMongoRcJs, int batchSize, QObject *parent) 
        : QObject(parent),
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

    void MongoWorker::saveDocument(EventsInfo::SaveDocumentInfo &inf)
    {
        ErrorInfo &er = inf._errorInfo;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try {
                if (inf._overwrite){
                    mongo::BSONElement id = inf._obj.getField("_id");
                    mongo::BSONObjBuilder builder;
                    builder.append(id);
                    mongo::BSONObj bsonQuery = builder.obj();
                    mongo::Query query(bsonQuery);

                    con->update(inf._obj.toString(), query, inf._obj, true, false);
                }                    
                else{
                    con->insert(inf._obj.toString(), inf._obj);
                }
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo(ex.what(), ErrorInfo::_EXCEPTION);
                LOG_MSG(er._description, mongo::LL_ERROR);
            }
        }
    }

    void MongoWorker::removeDocuments(EventsInfo::RemoveDocumentInfo &inf)
    {
        ErrorInfo &er = inf._errorInfo;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                con->remove(inf._ns.toString(), inf._query, inf._justOne);
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to remove documents.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }        
    }

    void MongoWorker::dropCollection(EventsInfo::DropCollectionInfo &inf)
    {
        ErrorInfo &er = inf._errorInfo;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                con->dropCollection(inf._ns.toString());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to drop collection.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }        
    }

    void MongoWorker::createDatabase(EventsInfo::CreateDataBaseInfo &inf)
    {
        ErrorInfo &er = inf._errorInfo;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                /*
                *  Here we are going to insert temp document to "<dbName>.temp" collection.
                *  This will create <dbName> database for us.
                *  Finally we are dropping just created temporary collection.
                */

                MongoNamespace ns(inf._database, "temp");

                // If <dbName>.temp already exists, stop.
                if (con->exists(ns.toString()))
                    return;

                // Building { _id : "temp" } document
                mongo::BSONObjBuilder builder;
                builder.append("_id", "temp");
                mongo::BSONObj obj = builder.obj();

                // Insert this document
                con->insert(ns.toString(), obj);

                // Drop temp collection
                con->dropCollection(ns.toString());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to create database.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }        
    }

    void MongoWorker::duplicateCollection(EventsInfo::DuplicateCollectionInfo &inf)
    {
        ErrorInfo &er = inf._errorInfo;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                MongoNamespace from(inf._ns);
                MongoNamespace to(inf._ns.databaseName(), inf._name);

                if (!con->exists(to.toString()))
                    con->createCollection(to.toString());

                std::auto_ptr<mongo::DBClientCursor> cursor(con->query(from.toString(), mongo::Query()));
                while (cursor->more()) {
                    mongo::BSONObj bsonObj = cursor->next();
                    con->insert(to.toString(), bsonObj);
                }
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to duplicate collection.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }        
    }

    void MongoWorker::copyCollectionToDiffServer(EventsInfo::CopyCollectionToDiffServerInfo &inf)
    {
        ErrorInfo &er = inf._errorInfo;
        mongo::DBClientBase *connectionTo = getConnection(er);
        if(!er.isError()){
            mongo::DBClientBase *connectionFrom = inf._server->_client->getConnection(er);
            if (!er.isError()){
                try{
                    if (!connectionTo->exists(inf._to.toString()))
                        connectionTo->createCollection(inf._to.toString());

                    std::auto_ptr<mongo::DBClientCursor> cursor(connectionFrom->query(inf._from.toString(), mongo::Query()));
                    while (cursor->more()) {
                        mongo::BSONObj bsonObj = cursor->next();
                        connectionFrom->insert(inf._to.toString(), bsonObj);
                    }
                } catch(const mongo::DBException &ex) {
                    er = ErrorInfo("Unable to copy collection.", ErrorInfo::_EXCEPTION);
                    LOG_MSG(ex.what(), mongo::LL_ERROR);
                }
            }
        }       
    }
        
    void MongoWorker::createCollection(EventsInfo::CreateCollectionInfo &inf)
    {
        ErrorInfo &er = inf._errorInfo;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                con->createCollection(inf._ns.toString());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to create collection.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
    }

    void MongoWorker::dropDatabase(EventsInfo::DropDatabaseInfo &inf)
    {
        ErrorInfo &er = inf._errorInfo;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                con->dropDatabase(inf._database);
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to drop database.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
    }

    void MongoWorker::establishConnection(EventsInfo::EstablishConnectionInfo &inf)
    {
        ErrorInfo &er = inf._errorInfo;
        QMutexLocker lock(&_firstConnectionMutex);

        mongo::DBClientBase *con = getConnection(er);
        std::vector<std::string> dbNames;
        float vers = 0.0f;

        if(!er.isError()){
            try {
                CredentialSettings primCred = _connection->primaryCredential();
                if (primCred.isValidAndEnabled()) {
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
                    dbNames = getDatabaseNames(er);
                    vers = getVersion(er);
                }
            } catch(const std::exception &ex) {
                er = ErrorInfo("Unable to connect to MongoDB", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }

        inf._info = ConnectionInfo(_connection->getFullAddress(), dbNames, vers);
    }

    void MongoWorker::getCollectionInfos(EventsInfo::LoadCollectionInfo &inf)
    {
        ErrorInfo &er = inf._errorInfo;
        std::vector<std::string> namespaces = getCollectionNames(inf._database, er);
        std::vector<MongoCollectionInfo> res;
        if (!er.isError()){
            try {
                for (std::vector<std::string>::const_iterator it = namespaces.begin(); it!=namespaces.end(); ++it) {

                    MongoNamespace mongons(*it);
                    mongo::BSONObjBuilder command; // { collStats: "db.collection", scale : 1 }
                    command.append("collStats", mongons.collectionName());
                    command.append("scale", 1);
                    mongo::BSONObj result;
                    _dbclient->runCommand(mongons.databaseName(), command.obj(), result);
                    MongoCollectionInfo info(result);
                    if (info.ns().isValid()){
                        res.push_back(info);
                    }            
                }
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to load list of collections.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
        inf._infos = res;
    }

    void MongoWorker::renameCollection(EventsInfo::RenameCollectionInfo &inf)
    {
        ErrorInfo &er = inf._errorInfo;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                MongoNamespace from(inf._ns);
                MongoNamespace to(inf._ns.databaseName(), inf._name);

                // Building { renameCollection: <source-namespace>, to: <target-namespace> }
                mongo::BSONObjBuilder command; // { collStats: "db.collection", scale : 1 }
                command.append("renameCollection", from.toString());
                command.append("to", to.toString());

                mongo::BSONObj result;
                con->runCommand("admin", command.obj(), result); // this command should be run against "admin" db
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to rename collection.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }       
    }  

    void MongoWorker::query(EventsInfo::ExecuteQueryInfo &inf)
    {
        ErrorInfo &er = inf._errorInfo;
        mongo::DBClientBase *con = getConnection(er);
        std::vector<MongoDocumentPtr> docs;
        if (!er.isError()){
            try{
                MongoNamespace ns(inf._queryInfo._collectionInfo._ns);

                //int limit = (info.limit <= 0) ? 50 : info.limit;

                if (inf._queryInfo._limit == -1) // it means that we do not need to load any documents
                    inf._documents = docs;

                std::auto_ptr<mongo::DBClientCursor> cursor = con->query(
                    ns.toString(), inf._queryInfo._query, inf._queryInfo._limit, inf._queryInfo._skip,
                    inf._queryInfo._fields.nFields() ? &inf._queryInfo._fields : 0, inf._queryInfo._options, inf._queryInfo._batchSize);

                while (cursor->more()) {
                    mongo::BSONObj bsonObj = cursor->next();
                    MongoDocumentPtr doc(new MongoDocument(bsonObj.getOwned()));
                    docs.push_back(doc);
                }    
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to complete query.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }       

        inf._documents = docs;
    }

    void MongoWorker::dropIndexFromCollection(EventsInfo::DeleteIndexInfo &inf)
    {
        ErrorInfo &er = inf._errorInfo;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try {
                con->dropIndex(inf._collection.ns().toString(), inf._name);
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo(ex.what(), ErrorInfo::_EXCEPTION);
                LOG_MSG(er._description, mongo::LL_ERROR);
            } 
        }
    }

    void MongoWorker::ensureIndex(EventsInfo::CreateIndexInfo &inf)
    {   
        ErrorInfo &er = inf._errorInfo;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()) {
            try {            
                std::string ns = inf._newIndex._collection.ns().toString();
                mongo::BSONObj keys = mongo::Robomongo::fromjson(inf._newIndex._request);
                mongo::BSONObjBuilder toSave;
                bool cache=true;
                int version =-1;

                toSave.append( "ns" , ns );
                toSave.append( "key" , keys );

                std::string cacheKey(ns);
                cacheKey += "--";


                if ( inf._newIndex._name != "" ) {
                    toSave.append( "name" , inf._newIndex._name );
                    cacheKey += inf._newIndex._name;
                }
                else {
                    std::string nn =  con->genIndexName(keys);
                    toSave.append( "name" , nn );
                    cacheKey += nn;
                }

                if (version >= 0)
                    toSave.append("v", version);

                if (inf._oldIndex._unique != inf._newIndex._unique)
                    toSave.appendBool("unique", inf._newIndex._unique);

                if (inf._oldIndex._backGround != inf._newIndex._backGround)
                    toSave.appendBool("background", inf._newIndex._backGround);

                if (inf._oldIndex._dropDups != inf._newIndex._dropDups)
                    toSave.appendBool("dropDups", inf._newIndex._dropDups);

                if (inf._oldIndex._sparse != inf._newIndex._sparse)
                    toSave.appendBool("sparse", inf._newIndex._sparse);

                if (inf._oldIndex._defaultLanguage != inf._newIndex._defaultLanguage)
                    toSave.append("default_language", inf._newIndex._defaultLanguage);

                if (inf._oldIndex._languageOverride != inf._newIndex._languageOverride)
                    toSave.append("language_override", inf._newIndex._languageOverride);

                if (inf._oldIndex._textWeights != inf._newIndex._textWeights)
                    toSave.append("weights", inf._newIndex._textWeights);

                /* if ( _seenIndexes.count( cacheKey ) )
                    return 0;

                if ( cache )
                    _seenIndexes.insert( cacheKey );*/

                if (inf._oldIndex._ttl != inf._newIndex._ttl)
                    toSave.append("expireAfterSeconds", inf._newIndex._ttl);

                MongoNamespace namesp(inf._newIndex._collection.ns().databaseName(), "system.indexes");
                mongo::BSONObj obj = toSave.obj();
                if (!inf._oldIndex._name.empty())
                    con->dropIndex(ns, inf._oldIndex._name);

                con->insert(namesp.toString().c_str(), obj);
            } catch(std::exception &ex) {
                er = ErrorInfo(ex.what(), ErrorInfo::_EXCEPTION);
                LOG_MSG(er._description, mongo::LL_ERROR);
            }
        }
    }

    void MongoWorker::getIndexes(EventsInfo::LoadCollectionIndexesInfo &inf)
    {
        ErrorInfo &er = inf._errorInfo;
        mongo::DBClientBase *con = getConnection(er);
        std::vector<EnsureIndex> result;
        if (!er.isError()){
            try {                
                std::auto_ptr<mongo::DBClientCursor> cursor(con->getIndexes(inf._collection.ns().toString()));

                while (cursor->more()) {
                    mongo::BSONObj bsonObj = cursor->next();
                    result.push_back(makeEnsureIndexInfoFromBsonObj(inf._collection,bsonObj));
                }
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo(ex.what(), ErrorInfo::_EXCEPTION);
                LOG_MSG(er._description, mongo::LL_ERROR);
            }
        }
        inf._indexes = result;
    }

    void MongoWorker::getFunctions(EventsInfo::LoadFunctionInfo &inf)
    {
        ErrorInfo &er = inf._errorInfo;
        mongo::DBClientBase *con = getConnection(er);
        const MongoNamespace ns(inf._database, "system.js");
        std::vector<MongoFunction> functions;

        if (!er.isError()){
            try{
                std::auto_ptr<mongo::DBClientCursor> cursor(con->query(ns.toString(), mongo::Query()));

                while (cursor->more()) {
                    mongo::BSONObj bsonObj = cursor->next();

                    try {
                        MongoFunction user(bsonObj);
                        functions.push_back(user);
                    } catch (const std::exception &) {
                        // skip invalid docs
                    }
                }
            } catch(const mongo::DBException &ex) { 
                er = ErrorInfo("Unable to load list of functions.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }        
        inf._functions = functions;
    }

    void MongoWorker::createFunction(EventsInfo::CreateFunctionInfo &inf)
    {
        ErrorInfo &er = inf._errorInfo;
        const MongoNamespace ns(inf._database, "system.js");
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                mongo::BSONObj obj = inf._function.toBson();

                if (inf._existingFunctionName.empty()) { // this is insert
                    con->insert(ns.toString(), obj);
                } else { // this is update

                    std::string name = inf._function.name();

                    if (inf._existingFunctionName == name) {
                        mongo::BSONObjBuilder builder;
                        builder.append("_id", name);
                        mongo::BSONObj bsonQuery = builder.obj();
                        mongo::Query query(bsonQuery);

                        con->update(ns.toString(), query, obj, true, false);
                    } else {
                        con->insert(ns.toString(), obj);
                        std::string res = con->getLastError();

                        // if no errors
                        if (res.empty()) {
                            mongo::BSONObjBuilder builder;
                            builder.append("_id", inf._existingFunctionName);
                            mongo::BSONObj bsonQuery = builder.obj();
                            mongo::Query query(bsonQuery);
                            con->remove(ns.toString(), query, true);
                        }
                    }
                }
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to create/ovewrite function.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
    }

    void MongoWorker::dropFunction(EventsInfo::DropFunctionInfo &inf)
    {
        ErrorInfo &er = inf._errorInfo;
        const MongoNamespace ns(inf._database, "system.js");
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                mongo::BSONObjBuilder builder;
                builder.append("_id", inf._name);
                mongo::BSONObj bsonQuery = builder.obj();
                mongo::Query query(bsonQuery);

                con->remove(ns.toString(), query, true);
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to drop function.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
    }

    void MongoWorker::getUsers(EventsInfo::LoadUserInfo &inf)
    {
        ErrorInfo &er = inf._errorInfo;
        MongoNamespace ns(inf._database, "system.users");
        std::vector<MongoUser> users;

        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try {            
                std::auto_ptr<mongo::DBClientCursor> cursor(con->query(ns.toString(), mongo::Query()));
                float ver = getVersion(er);
                while (cursor->more()) {
                    mongo::BSONObj bsonObj = cursor->next();
                    MongoUser user(ver,bsonObj);
                    users.push_back(user);
                }           
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to load list of users.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }  
        inf._users = users;
    }

    void MongoWorker::createUser(EventsInfo::CreateUserInfo &inf)
    {
        ErrorInfo &er = inf._errorInfo;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                MongoNamespace ns(inf._database, "system.users");
                mongo::BSONObj obj = inf._user.toBson();

                if (!inf._overwrite) {
                    con->insert(ns.toString(), obj);
                } else {
                    mongo::BSONElement id = obj.getField("_id");
                    mongo::BSONObjBuilder builder;
                    builder.append(id);
                    mongo::BSONObj bsonQuery = builder.obj();
                    mongo::Query query(bsonQuery);

                    con->update(ns.toString(), query, obj, true, false);
                }
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to create/ovewrite user.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }       
    }

    void MongoWorker::dropUser(EventsInfo::DropUserInfo &inf)
    {
        ErrorInfo &er = inf._errorInfo;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                MongoNamespace ns(inf._database, "system.users");

                mongo::BSONObjBuilder builder;
                builder.append("_id", inf._id);
                mongo::BSONObj bsonQuery = builder.obj();
                mongo::Query query(bsonQuery);

                con->remove(ns.toString(), query, true);
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to drop user.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }        
    }

    std::vector<std::string> MongoWorker::getCollectionNames(const std::string &dbname, ErrorInfo &er)
    {
        std::vector<std::string> stringList;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try {
                typedef std::list<std::string> cont_string_t;
                cont_string_t dbs = con->getCollectionNames(dbname);                
                for (cont_string_t::const_iterator i = dbs.begin(); i != dbs.end(); i++) {
                    stringList.push_back(*i);
                }
                std::sort(stringList.begin(), stringList.end());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to load list of collections.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
        return stringList;
    }

    std::vector<std::string> MongoWorker::getDatabaseNames(ErrorInfo &er)
    {            
        std::vector<std::string> dbNames;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){

            std::string authBase = getAuthBase();
            if (!_isAdmin && !authBase.empty()) {
                dbNames.push_back(authBase);
                return dbNames;
            }

            try { 
                typedef std::list<std::string> cont_string_t;
                cont_string_t dbs = con->getDatabaseNames();        
                for (cont_string_t::const_iterator i = dbs.begin(); i != dbs.end(); ++i) {
                    dbNames.push_back(*i);
                }
                std::sort(dbNames.begin(), dbNames.end());
            }
            catch(const std::exception &ex)
            {
                if(!authBase.empty())
                    dbNames.push_back(authBase);

                er = ErrorInfo(ex.what(), ErrorInfo::_EXCEPTION);
                LOG_MSG(er._description, mongo::LL_ERROR);
            }    
        }

        return dbNames;
    }

    float MongoWorker::getVersion(ErrorInfo &er)
    {
        float result = 0.0f;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try {            
                mongo::BSONObj resultObj;
                con->runCommand("db", BSON("buildInfo" << "1"), resultObj);
                std::string resultStr = BsonUtils::getField<mongo::String>(resultObj,"version");
                resultStr.erase(std::remove(resultStr.begin()+2 ,resultStr.end(),'.'));
                result = atof(resultStr.c_str());          
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to load list of users.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
        return result;
    }

    void MongoWorker::customEvent(QEvent *event)// handlers
    {
        using namespace Events;
        QEvent::Type type = event->type();
        if (type==static_cast<QEvent::Type>(RemoveDocumentEvent::EventType)){
            RemoveDocumentEvent *ev = static_cast<RemoveDocumentEvent*>(event);
            RemoveDocumentEvent::value_type v = ev->value();
    
            removeDocuments(v);
            qApp->postEvent(ev->sender(), new RemoveDocumentEvent(this, v));
        }
        else if (type==static_cast<QEvent::Type>(SaveDocumentEvent::EventType)){
            SaveDocumentEvent *ev = static_cast<SaveDocumentEvent*>(event);
            SaveDocumentEvent::value_type v = ev->value();

            saveDocument(v);            
            qApp->postEvent(ev->sender(), new SaveDocumentEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(DropFunctionEvent::EventType))
        {
            DropFunctionEvent *ev = static_cast<DropFunctionEvent*>(event);
            DropFunctionEvent::value_type v = ev->value();

            dropFunction(v);
            qApp->postEvent(ev->sender(), new DropFunctionEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(CreateFunctionEvent::EventType))
        {
            CreateFunctionEvent *ev = static_cast<CreateFunctionEvent*>(event);
            CreateFunctionEvent::value_type v = ev->value();            

            createFunction(v);
            qApp->postEvent(ev->sender(), new CreateFunctionEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(LoadFunctionEvent::EventType))
        {
            LoadFunctionEvent *ev = static_cast<LoadFunctionEvent*>(event);
            LoadFunctionEvent::value_type v = ev->value();            

            getFunctions(v);
            qApp->postEvent(ev->sender(), new LoadFunctionEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(CreateUserEvent::EventType))
        {
            CreateUserEvent *ev = static_cast<CreateUserEvent*>(event);
            CreateUserEvent::value_type v = ev->value();  
            
            createUser(v);
            qApp->postEvent(ev->sender(), new CreateUserEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(DropUserEvent::EventType))
        {
            DropUserEvent *ev = static_cast<DropUserEvent*>(event);
            DropUserEvent::value_type v = ev->value();  

            ErrorInfo er;
            dropUser(v);
            qApp->postEvent(ev->sender(), new DropUserEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(LoadUserEvent::EventType))
        {
            LoadUserEvent *ev = static_cast<LoadUserEvent*>(event);
            LoadUserEvent::value_type v = ev->value();            

            getUsers(v);
            qApp->postEvent(ev->sender(), new LoadUserEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(CreateCollectionEvent::EventType)){
            CreateCollectionEvent *ev = static_cast<CreateCollectionEvent*>(event);
            CreateCollectionEvent::value_type v = ev->value();  

            createCollection(v);
            qApp->postEvent(ev->sender(), new CreateCollectionEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(DropCollectionEvent::EventType)){
            DropCollectionEvent *ev = static_cast<DropCollectionEvent*>(event);
            DropCollectionEvent::value_type v = ev->value();  

            dropCollection(v);
            qApp->postEvent(ev->sender(), new DropCollectionEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(RenameCollectionEvent::EventType)){
            RenameCollectionEvent *ev = static_cast<RenameCollectionEvent*>(event);
            RenameCollectionEvent::value_type v = ev->value();  

            renameCollection(v);
            qApp->postEvent(ev->sender(), new RenameCollectionEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(LoadCollectionEvent::EventType))
        {
            LoadCollectionEvent *ev = static_cast<LoadCollectionEvent*>(event);
            LoadCollectionEvent::value_type v = ev->value();            

            getCollectionInfos(v);
            qApp->postEvent(ev->sender(), new LoadCollectionEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(DuplicateCollectionEvent::EventType)){
            DuplicateCollectionEvent *ev = static_cast<DuplicateCollectionEvent*>(event);
            DuplicateCollectionEvent::value_type v = ev->value();            

            ErrorInfo er;
            duplicateCollection(v);
            qApp->postEvent(ev->sender(), new DuplicateCollectionEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(CopyCollectionToDiffServerEvent::EventType)){
            CopyCollectionToDiffServerEvent *ev = static_cast<CopyCollectionToDiffServerEvent*>(event);
            CopyCollectionToDiffServerEvent::value_type v = ev->value();            

            copyCollectionToDiffServer(v);
            qApp->postEvent(ev->sender(), new CopyCollectionToDiffServerEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(LoadCollectionIndexEvent::EventType)){
            LoadCollectionIndexEvent *ev = static_cast<LoadCollectionIndexEvent*>(event);
            LoadCollectionIndexEvent::value_type v = ev->value();            

            getIndexes(v);
            qApp->postEvent(ev->sender(), new LoadCollectionIndexEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(CreateIndexEvent::EventType)){
            CreateIndexEvent *ev = static_cast<CreateIndexEvent*>(event);
            CreateIndexEvent::value_type v = ev->value();            

            ensureIndex(v);
            qApp->postEvent(ev->sender(), new CreateIndexEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(DeleteIndexEvent::EventType)){
            DeleteIndexEvent *ev = static_cast<DeleteIndexEvent*>(event);
            DeleteIndexEvent::value_type v = ev->value();            

            dropIndexFromCollection(v);
            qApp->postEvent(ev->sender(), new DeleteIndexEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(CreateDataBaseEvent::EventType)){
            CreateDataBaseEvent *ev = static_cast<CreateDataBaseEvent*>(event);
            CreateDataBaseEvent::value_type v = ev->value();            

            createDatabase(v);
            qApp->postEvent(ev->sender(), new CreateDataBaseEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(DropDatabaseEvent::EventType)){
            DropDatabaseEvent *ev = static_cast<DropDatabaseEvent*>(event);
            DropDatabaseEvent::value_type v = ev->value();            

            dropDatabase(v);
            qApp->postEvent(ev->sender(), new DropDatabaseEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(LoadDatabaseNamesEvent::EventType)){
            LoadDatabaseNamesEvent *ev = static_cast<LoadDatabaseNamesEvent*>(event);
            LoadDatabaseNamesEvent::value_type v = ev->value();            

            ErrorInfo er;
            v._databaseNames = getDatabaseNames(er);
            qApp->postEvent(ev->sender(), new LoadDatabaseNamesEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(AutoCompleteEvent::EventType)){
            AutoCompleteEvent *ev = static_cast<AutoCompleteEvent*>(event);
            AutoCompleteEvent::value_type v = ev->value();            

            try {
                v._list = _scriptEngine->complete(v._prefix);            
            } catch(const mongo::DBException &ex) {
                v._errorInfo = ErrorInfo("Unable to autocomplete query.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
            qApp->postEvent(ev->sender(), new AutoCompleteEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(ExecuteQueryEvent::EventType)){
            ExecuteQueryEvent *ev = static_cast<ExecuteQueryEvent*>(event);
            ExecuteQueryEvent::value_type v = ev->value();            

            query(v);
            qApp->postEvent(ev->sender(), new ExecuteQueryEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(ExecuteScriptEvent::EventType)){
            ExecuteScriptEvent *ev = static_cast<ExecuteScriptEvent*>(event);
            ExecuteScriptEvent::value_type v = ev->value();            

            try {
                v._result = _scriptEngine->exec(v._script, v._databaseName);   
            } catch(const mongo::DBException &ex) {
                v._errorInfo = ErrorInfo("Unable to complete query.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
            qApp->postEvent(ev->sender(), new ExecuteScriptEvent(this, v));
        }
        else if(type==static_cast<QEvent::Type>(EstablishConnectionEvent::EventType)){
            EstablishConnectionEvent *ev = static_cast<EstablishConnectionEvent*>(event);
            EstablishConnectionEvent::value_type v = ev->value();            

            establishConnection(v);
            qApp->postEvent(ev->sender(), new EstablishConnectionEvent(this, v));
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
                    con->runCommand("admin", command.obj(), result);
                } else {
                    con->runCommand(authBase, command.obj(), result);
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

    void MongoWorker::init() //call once in MongoWorker thread
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

    std::string MongoWorker::getAuthBase() const
    {
        CredentialSettings primCred = _connection->primaryCredential();
        if (primCred.isValidAndEnabled()) {
            CredentialSettings::CredentialInfo inf = primCred.info();
            return inf._databaseName;
        }
        return std::string();
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
            er._description = "Unable to connect " + _connection->getFullAddress();
        }
        return _dbclient;
    }
}