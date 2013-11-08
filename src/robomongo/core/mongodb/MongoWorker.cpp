#include "robomongo/core/mongodb/MongoWorker.h"

#include <QThread>
#include <QApplication>

#include "robomongo/core/EventBus.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/domain/MongoCollectionInfo.h"
#include "robomongo/core/events/MongoEventsGui.hpp"
#include "robomongo/core/engine/ScriptEngine.h"
#include "robomongo/core/utils/Logger.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/utils/BsonUtils.h"

#include "robomongo/shell/db/json.h"
#include <mongo/client/dbclient_rs.h>

namespace
{
    Robomongo::EnsureIndexInfo makeEnsureIndexInfoFromBsonObj(
        const Robomongo::MongoCollectionInfo &collection,
        const mongo::BSONObj &obj)
    {
        using namespace Robomongo::BsonUtils;
        Robomongo::EnsureIndexInfo info(collection);
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

    void MongoWorker::saveDocument(const mongo::BSONObj &obj, const MongoNamespace &ns, bool overwrite, ErrorInfo &er)
    {
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try {
                if (overwrite){
                    mongo::BSONElement id = obj.getField("_id");
                    mongo::BSONObjBuilder builder;
                    builder.append(id);
                    mongo::BSONObj bsonQuery = builder.obj();
                    mongo::Query query(bsonQuery);

                    con->update(ns.toString(), query, obj, true, false);
                }                    
                else{
                    con->insert(ns.toString(), obj);
                }
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo(ex.what(), ErrorInfo::_EXCEPTION);
                LOG_MSG(er._description, mongo::LL_ERROR);
            }
        }
    }

    void MongoWorker::removeDocuments(const MongoNamespace &ns, mongo::Query query, bool justOne, ErrorInfo &er)
    {
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                con->remove(ns.toString(), query, justOne);
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to remove documents.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }        
    }

    void MongoWorker::dropCollection(const MongoNamespace &ns, ErrorInfo &er)
    {
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                con->dropCollection(ns.toString());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to drop collection.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }        
    }

    void MongoWorker::duplicateCollection(const MongoNamespace &ns, const std::string &newCollectionName, ErrorInfo &er)
    {
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                MongoNamespace from(ns);
                MongoNamespace to(ns.databaseName(), newCollectionName);

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

    void MongoWorker::copyCollectionToDiffServer(MongoWorker *cl, const MongoNamespace &from, const MongoNamespace &to, ErrorInfo &er)
    {
        mongo::DBClientBase *connectionTo = getConnection(er);
        if(!er.isError()){
            mongo::DBClientBase *connectionFrom = cl->getConnection(er);
            if (!er.isError()){
                try{
                    if (!connectionTo->exists(to.toString()))
                        connectionTo->createCollection(to.toString());

                    std::auto_ptr<mongo::DBClientCursor> cursor(connectionFrom->query(from.toString(), mongo::Query()));
                    while (cursor->more()) {
                        mongo::BSONObj bsonObj = cursor->next();
                        connectionFrom->insert(to.toString(), bsonObj);
                    }
                } catch(const mongo::DBException &ex) {
                    er = ErrorInfo("Unable to copy collection.", ErrorInfo::_EXCEPTION);
                    LOG_MSG(ex.what(), mongo::LL_ERROR);
                }
            }
        }       
    }
        
    void MongoWorker::createCollection(const MongoNamespace &ns, ErrorInfo &er)
    {
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                con->createCollection(ns.toString());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to create collection.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
    }

    void MongoWorker::renameCollection(const MongoNamespace &ns, const std::string &newCollectionName, ErrorInfo &er)
    {
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                MongoNamespace from(ns);
                MongoNamespace to(ns.databaseName(), newCollectionName);

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

    std::vector<MongoCollectionInfo> MongoWorker::getCollectionInfos(const std::string &dbname, ErrorInfo &er)
    {
        std::vector<MongoCollectionInfo> infos;
        std::vector<std::string> namespaces = getCollectionNames(dbname, er);
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
                        infos.push_back(info);
                    }            
                }
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to load list of collections.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
        return infos;
    }

    void MongoWorker::dropDatabase(const std::string &dbName, ErrorInfo &er)
    {
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                con->dropDatabase(dbName);
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to drop database.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
    }
    
    void MongoWorker::createDatabase(const std::string &dbName, ErrorInfo &er)
    {
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                /*
                *  Here we are going to insert temp document to "<dbName>.temp" collection.
                *  This will create <dbName> database for us.
                *  Finally we are dropping just created temporary collection.
                */

                MongoNamespace ns(dbName, "temp");

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

    std::vector<MongoDocumentPtr> MongoWorker::query(const MongoQueryInfo &info, ErrorInfo &er)
    {
        mongo::DBClientBase *con = getConnection(er);
        std::vector<MongoDocumentPtr> docs;
        if (!er.isError()){
            try{
                MongoNamespace ns(info._collectionInfo._ns);

                //int limit = (info.limit <= 0) ? 50 : info.limit;

                if (info._limit == -1) // it means that we do not need to load any documents
                    return docs;

                std::auto_ptr<mongo::DBClientCursor> cursor = con->query(
                    ns.toString(), info._query, info._limit, info._skip,
                    info._fields.nFields() ? &info._fields : 0, info._options, info._batchSize);

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

        return docs;
    }

    void MongoWorker::dropIndexFromCollection(const MongoCollectionInfo &collection, const std::string &indexName, ErrorInfo &er)
    {
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try {
                con->dropIndex(collection.ns().toString(), indexName);
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo(ex.what(), ErrorInfo::_EXCEPTION);
                LOG_MSG(er._description, mongo::LL_ERROR);
            } 
        }
    }

    std::vector<EnsureIndexInfo> MongoWorker::getIndexes(const MongoCollectionInfo &collection, ErrorInfo &er)
    {
        mongo::DBClientBase *con = getConnection(er);
        std::vector<EnsureIndexInfo> result;
        if (!er.isError()){
            try {                
                std::auto_ptr<mongo::DBClientCursor> cursor(con->getIndexes(collection.ns().toString()));

                while (cursor->more()) {
                    mongo::BSONObj bsonObj = cursor->next();
                    result.push_back(makeEnsureIndexInfoFromBsonObj(collection,bsonObj));
                }
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo(ex.what(), ErrorInfo::_EXCEPTION);
                LOG_MSG(er._description, mongo::LL_ERROR);
            }
        }
        return result;
    }

    void MongoWorker::renameIndexFromCollection(const MongoCollectionInfo &collection, const std::string &oldIndexName, const std::string &newIndexName, ErrorInfo &er)
    {
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                // This is simply an example of how to perform modifications of
                // BSON objects. Because BSONObj is immutable, you need to create
                // copy of this object, using BSONObjBuilder and BSONObjIterator.
                //
                // But we need to do not just simple renaming of Index name, we
                // also should allow our users to fully modify Index
                // (i.e. change name, keys, unique flag, sparse flag etc.)
                //
                // This should be done using the same dialog as for "Add Index".

                MongoNamespace ns(collection.ns().databaseName(), "system.indexes");
                std::string systemIndexesNs = ns.toString();

                // Building this JSON: { "name" : "oldIndexName" }
                mongo::BSONObj query(mongo::BSONObjBuilder()
                    .append("name", oldIndexName)
                    .obj());

                // Searching for index with "oldIndexName"
                // with this query: db.system.indexes.find({ name : "oldIndexName"}
                mongo::BSONObj indexBson = con->findOne(systemIndexesNs, mongo::Query(query));
                if (indexBson.isEmpty())
                    return;

                // Here we are building copy of "indexBson" object and
                // changing "name" field's value from "oldIndexText" to "newIndexText":
                mongo::BSONObjBuilder builder;
                mongo::BSONObjIterator i(indexBson);
                while (i.more()) {
                    mongo::BSONElement element = i.next();

                    if (mongo::StringData(element.fieldName()).compare("name") == 0) {
                        builder.append("name", newIndexName);
                        continue;
                    }

                    builder.append(element);
                }
                std::string collectionNs = collection.ns().toString();

                con->dropIndex(collectionNs, oldIndexName);
                con->insert(systemIndexesNs, builder.obj());
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo(ex.what(), ErrorInfo::_EXCEPTION);
                LOG_MSG(er._description, mongo::LL_ERROR);
            } 
        }        
    }

    void MongoWorker::ensureIndex(const EnsureIndexInfo &oldInfo,const EnsureIndexInfo &newInfo, ErrorInfo &er)
    {   
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()) {
            try {            
                std::string ns = newInfo._collection.ns().toString();
                mongo::BSONObj keys = mongo::Robomongo::fromjson(newInfo._request);
                mongo::BSONObjBuilder toSave;
                bool cache=true;
                int version =-1;

                toSave.append( "ns" , ns );
                toSave.append( "key" , keys );

                std::string cacheKey(ns);
                cacheKey += "--";


                if ( newInfo._name != "" ) {
                    toSave.append( "name" , newInfo._name );
                    cacheKey += newInfo._name;
                }
                else {
                    std::string nn =  con->genIndexName(keys);
                    toSave.append( "name" , nn );
                    cacheKey += nn;
                }

                if (version >= 0)
                    toSave.append("v", version);

                if (oldInfo._unique != newInfo._unique)
                    toSave.appendBool("unique", newInfo._unique);

                if (oldInfo._backGround != newInfo._backGround)
                    toSave.appendBool("background", newInfo._backGround);

                if (oldInfo._dropDups != newInfo._dropDups)
                    toSave.appendBool("dropDups", newInfo._dropDups);

                if (oldInfo._sparse != newInfo._sparse)
                    toSave.appendBool("sparse", newInfo._sparse);

                if (oldInfo._defaultLanguage != newInfo._defaultLanguage)
                    toSave.append("default_language", newInfo._defaultLanguage);

                if (oldInfo._languageOverride != newInfo._languageOverride)
                    toSave.append("language_override", newInfo._languageOverride);

                if (oldInfo._textWeights != newInfo._textWeights)
                    toSave.append("weights", newInfo._textWeights);

                /* if ( _seenIndexes.count( cacheKey ) )
                    return 0;

                if ( cache )
                    _seenIndexes.insert( cacheKey );*/

                if (oldInfo._ttl != newInfo._ttl)
                    toSave.append("expireAfterSeconds", newInfo._ttl);

                MongoNamespace namesp(newInfo._collection.ns().databaseName(), "system.indexes");
                mongo::BSONObj obj = toSave.obj();
                if (!oldInfo._name.empty())
                    con->dropIndex(ns, oldInfo._name);

                con->insert(namesp.toString().c_str(), obj);
            } catch(std::exception &ex) {
                er = ErrorInfo(ex.what(), ErrorInfo::_EXCEPTION);
                LOG_MSG(er._description, mongo::LL_ERROR);
            }
        }
    }

    std::vector<MongoFunction> MongoWorker::getFunctions(const std::string &dbName, ErrorInfo &er)
    {
        mongo::DBClientBase *con = getConnection(er);
        const MongoNamespace ns(dbName, "system.js");
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
        return functions;
    }

    void MongoWorker::createFunction(const std::string &dbName, const MongoFunction &fun, const std::string &existingFunctionName, ErrorInfo &er)
    {
        const MongoNamespace ns(dbName, "system.js");
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                mongo::BSONObj obj = fun.toBson();

                if (existingFunctionName.empty()) { // this is insert
                    con->insert(ns.toString(), obj);
                } else { // this is update

                    std::string name = fun.name();

                    if (existingFunctionName == name) {
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
                            builder.append("_id", existingFunctionName);
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

    void MongoWorker::dropFunction(const std::string &dbName, const std::string &name, ErrorInfo &er)
    {
        const MongoNamespace ns(dbName, "system.js");
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                mongo::BSONObjBuilder builder;
                builder.append("_id", name);
                mongo::BSONObj bsonQuery = builder.obj();
                mongo::Query query(bsonQuery);

                con->remove(ns.toString(), query, true);
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to drop function.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
    }

    std::vector<MongoUser> MongoWorker::getUsers(const std::string &dbName, ErrorInfo &er)
    {
        MongoNamespace ns(dbName, "system.users");
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
        return users;
    }

    void MongoWorker::createUser(const std::string &dbName, const MongoUser &user, bool overwrite, ErrorInfo &er)
    {
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                MongoNamespace ns(dbName, "system.users");
                mongo::BSONObj obj = user.toBson();

                if (!overwrite) {
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

    float MongoWorker::getVersion(ErrorInfo &er)
    {
        float result = 0.0f;
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try {            
                mongo::BSONObj resultObj;
                con->runCommand("db", BSON("buildInfo" << "1"), resultObj);
                std::string resultStr = BsonUtils::getField<mongo::String>(resultObj,"version");
                result = atof(resultStr.c_str());          
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to load list of users.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
            }
        }
        return result;
    }

    void MongoWorker::dropUser(const std::string &dbName, const mongo::OID &id, ErrorInfo &er)
    {
        mongo::DBClientBase *con = getConnection(er);
        if (!er.isError()){
            try{
                MongoNamespace ns(dbName, "system.users");

                mongo::BSONObjBuilder builder;
                builder.append("_id", id);
                mongo::BSONObj bsonQuery = builder.obj();
                mongo::Query query(bsonQuery);

                con->remove(ns.toString(), query, true);
            } catch(const mongo::DBException &ex) {
                er = ErrorInfo("Unable to drop user.", ErrorInfo::_EXCEPTION);
                LOG_MSG(ex.what(), mongo::LL_ERROR);
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
            saveDocument(v._obj, v._ns, v._overwrite, er);            
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

        reply(event->sender(), new EstablishConnectionResponse(this, ConnectionInfo(_connection->getFullAddress(), dbNames, vers), er));
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

    /**
     * @brief Load list of all database names
     */
    void MongoWorker::handle(LoadDatabaseNamesRequest *event)
    {
        // If user not an admin - he doesn't have access to mongodb 'listDatabases' command
        // Non admin user has access only to the single database he specified while performing auth.
        ErrorInfo er;
        std::vector<std::string> dbNames = getDatabaseNames(er);
        reply(event->sender(), new LoadDatabaseNamesResponse(this, dbNames, er));
    }

    /**
     * @brief Load list of all collection names
     */
    void MongoWorker::handle(LoadCollectionNamesRequest *event)
    {
        ErrorInfo er;
        std::vector<MongoCollectionInfo> infos = getCollectionInfos(event->databaseName(), er);
        reply(event->sender(), new LoadCollectionNamesResponse(this, event->databaseName(), infos, er));
    }

    void MongoWorker::handle(LoadUsersRequest *event)
    {
        ErrorInfo er;
        std::vector<MongoUser> users = getUsers(event->databaseName(), er);
        reply(event->sender(), new LoadUsersResponse(this, event->databaseName(), users, er));
    }

    void MongoWorker::handle(LoadCollectionIndexesRequest *event)
    {
        ErrorInfo er;
        std::vector<EnsureIndexInfo> ind = getIndexes(event->collection(), er);
        reply(event->sender(), new LoadCollectionIndexesResponse(this, ind, er));
    }

    void MongoWorker::handle(EnsureIndexRequest *event)
    {
        const EnsureIndexInfo &newInfo = event->newInfo();
        const EnsureIndexInfo &oldInfo = event->oldInfo();
        ErrorInfo er;
        std::vector<EnsureIndexInfo> ind;
        ensureIndex(oldInfo, newInfo, er);
        if(er.isError()){
            ind = getIndexes(newInfo._collection, er);
        }
        reply(event->sender(), new LoadCollectionIndexesResponse(this, ind, er));
    }

    void MongoWorker::handle(DropCollectionIndexRequest *event)
    {
        ErrorInfo er;
        dropIndexFromCollection(event->collection(),event->name(), er);
        reply(event->sender(), new DropCollectionIndexResponse(this, event->collection(), event->name(), er));
    }

    void MongoWorker::handle(EditIndexRequest *event)
    {
        ErrorInfo er;
        renameIndexFromCollection(event->collection(), event->oldIndex(), event->newIndex(), er);
        std::vector<EnsureIndexInfo> ind;
        if (!er.isError()){
            ind = getIndexes(event->collection(),er);
        }
        reply(event->sender(), new LoadCollectionIndexesResponse(this, ind, er));
    }

    void MongoWorker::handle(LoadFunctionsRequest *event)
    {
        ErrorInfo er;
        std::vector<MongoFunction> funs = getFunctions(event->databaseName(), er);
        reply(event->sender(), new LoadFunctionsResponse(this,event->databaseName(), funs, er));
    }

    void MongoWorker::handle(InsertDocumentRequest *event)
    {
        ErrorInfo er;
        saveDocument(event->obj(), event->ns(), event->overwrite(), er);
        reply(event->sender(), new InsertDocumentResponse(this,er));
    }

    void MongoWorker::handle(RemoveDocumentRequest *event)
    {
        ErrorInfo er;
        removeDocuments(event->ns(), event->query(), event->justOne(),er);
        reply(event->sender(), new RemoveDocumentResponse(this, er));
    }

    void MongoWorker::handle(ExecuteQueryRequest *event)
    {
        ErrorInfo er;
        std::vector<MongoDocumentPtr> docs = query(event->queryInfo(), er);
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
        createDatabase(event->database(), er);
        reply(event->sender(), new CreateDatabaseResponse(this, er));
    }

    void MongoWorker::handle(DropDatabaseRequest *event)
    {
        ErrorInfo er;
        dropDatabase(event->database(), er);
        reply(event->sender(), new DropDatabaseResponse(this, er));
    }

    void MongoWorker::handle(CreateCollectionRequest *event)
    {
        ErrorInfo er;
        createCollection(event->ns(), er);
        reply(event->sender(), new CreateCollectionResponse(this, er));
    }

    void MongoWorker::handle(DropCollectionRequest *event)
    {
        ErrorInfo er;        
        dropCollection(event->ns(), er);
        reply(event->sender(), new DropCollectionResponse(this, er));
    }

    void MongoWorker::handle(RenameCollectionRequest *event)
    {
        ErrorInfo er;        
        renameCollection(event->ns(), event->newCollection(), er);
        reply(event->sender(), new RenameCollectionResponse(this, er));
    }

    void MongoWorker::handle(DuplicateCollectionRequest *event)
    {
        ErrorInfo er;
        duplicateCollection(event->ns(), event->newCollection(), er);
        reply(event->sender(), new DuplicateCollectionResponse(this, er));
    }

    void MongoWorker::handle(CopyCollectionToDiffServerRequest *event)
    {
        ErrorInfo er;
        copyCollectionToDiffServer(event->worker(), event->from(), event->to(), er);
        reply(event->sender(), new CopyCollectionToDiffServerResponse(this, er));
    }

    void MongoWorker::handle(CreateUserRequest *event)
    {
        ErrorInfo er;
        createUser(event->database(), event->user(), event->overwrite(), er);
        reply(event->sender(), new CreateUserResponse(this, er));
    }

    void MongoWorker::handle(DropUserRequest *event)
    {
        ErrorInfo er;
        dropUser(event->database(), event->id(), er);
        reply(event->sender(), new DropUserResponse(this, er));
    }

    void MongoWorker::handle(CreateFunctionRequest *event)
    {
        ErrorInfo er;
        createFunction(event->database(), event->function(), event->existingFunctionName(), er);
        reply(event->sender(), new CreateFunctionResponse(this, er));
    }

    void MongoWorker::handle(DropFunctionRequest *event)
    {
        ErrorInfo er;
        dropFunction(event->database(), event->name(), er);
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
            er._description = "Unable to connect " + _connection->getFullAddress();
        }
        return _dbclient;
    }

    /**
     * @brief Send reply event to object 'receiver'
     */
    void MongoWorker::reply(QObject *receiver, Event *event)
    {
        AppRegistry::instance().bus()->send(receiver, event);
    }
}