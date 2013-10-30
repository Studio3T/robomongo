#include "robomongo/core/mongodb/MongoClient.h"

#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/shell/db/json.h"

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
    MongoClient::MongoClient(mongo::DBClientConnection *const dbclient) :
        _dbclient(dbclient) { }

    std::vector<std::string> MongoClient::getCollectionNames(const std::string &dbname) const
    {
        typedef std::list<std::string> cont_string_t;
        cont_string_t dbs = _dbclient->getCollectionNames(dbname);

        std::vector<std::string> stringList;
        for (cont_string_t::const_iterator i = dbs.begin(); i != dbs.end(); i++) {
            stringList.push_back(*i);
        }
        std::sort(stringList.begin(), stringList.end());
        return stringList;
    }


    float MongoClient::getVersion() const
    {
        float result = 0.0f;
        mongo::BSONObj resultObj;
        _dbclient->runCommand("db", BSON("buildInfo" << "1"), resultObj);
        std::string resultStr = BsonUtils::getField<mongo::String>(resultObj,"version");
        result = atof(resultStr.c_str());
        return result;
    }

    std::vector<std::string> MongoClient::getDatabaseNames() const
    {
        typedef std::list<std::string> cont_string_t;
        cont_string_t dbs = _dbclient->getDatabaseNames();
        std::vector<std::string> dbNames;
        for (cont_string_t::const_iterator i = dbs.begin(); i != dbs.end(); ++i) {
            dbNames.push_back(*i);
        }
        std::sort(dbNames.begin(), dbNames.end());
        return dbNames;
    }

    std::vector<MongoUser> MongoClient::getUsers(const std::string &dbName)
    {
        MongoNamespace ns(dbName, "system.users");
        std::vector<MongoUser> users;

        std::auto_ptr<mongo::DBClientCursor> cursor(_dbclient->query(ns.toString(), mongo::Query()));
        float ver = getVersion();
        while (cursor->more()) {
            mongo::BSONObj bsonObj = cursor->next();
            MongoUser user(ver,bsonObj);
            users.push_back(user);
        }

        return users;
    }

    void MongoClient::createUser(const std::string &dbName, const MongoUser &user, bool overwrite)
    {
        MongoNamespace ns(dbName, "system.users");
        mongo::BSONObj obj = user.toBson();

        if (!overwrite) {
            _dbclient->insert(ns.toString(), obj);
        } else {
            mongo::BSONElement id = obj.getField("_id");
            mongo::BSONObjBuilder builder;
            builder.append(id);
            mongo::BSONObj bsonQuery = builder.obj();
            mongo::Query query(bsonQuery);

            _dbclient->update(ns.toString(), query, obj, true, false);
        }
    }

    void MongoClient::dropUser(const std::string &dbName, const mongo::OID &id)
    {
        MongoNamespace ns(dbName, "system.users");

        mongo::BSONObjBuilder builder;
        builder.append("_id", id);
        mongo::BSONObj bsonQuery = builder.obj();
        mongo::Query query(bsonQuery);

        _dbclient->remove(ns.toString(), query, true);
    }

    std::vector<MongoFunction> MongoClient::getFunctions(const std::string &dbName)
    {
        MongoNamespace ns(dbName, "system.js");
        std::vector<MongoFunction> functions;

        std::auto_ptr<mongo::DBClientCursor> cursor(_dbclient->query(ns.toString(), mongo::Query()));

        while (cursor->more()) {
            mongo::BSONObj bsonObj = cursor->next();

            try {
                MongoFunction user(bsonObj);
                functions.push_back(user);
            } catch (const std::exception &) {
                // skip invalid docs
            }
        }
        return functions;
    }

    std::vector<EnsureIndexInfo> MongoClient::getIndexes(const MongoCollectionInfo &collection) const
    {
        std::vector<EnsureIndexInfo> result;
        std::auto_ptr<mongo::DBClientCursor> cursor(_dbclient->getIndexes(collection.ns().toString()));

        while (cursor->more()) {
            mongo::BSONObj bsonObj = cursor->next();
            result.push_back(makeEnsureIndexInfoFromBsonObj(collection,bsonObj));
        }

        return result;
    }

    void MongoClient::ensureIndex(const EnsureIndexInfo &oldInfo,const EnsureIndexInfo &newInfo) const
    {   
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
            std::string nn =  _dbclient->genIndexName(keys);
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
            _dbclient->dropIndex(ns, oldInfo._name);

        _dbclient->insert(namesp.toString().c_str(), obj);
    }

    void MongoClient::renameIndexFromCollection(const MongoCollectionInfo &collection, const std::string &oldIndexName, const std::string &newIndexName) const
    {
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
        mongo::BSONObj indexBson = _dbclient->findOne(systemIndexesNs, mongo::Query(query));
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

        _dbclient->dropIndex(collectionNs, oldIndexName);
        _dbclient->insert(systemIndexesNs, builder.obj());
    }

    void MongoClient::dropIndexFromCollection(const MongoCollectionInfo &collection, const std::string &indexName) const
    {
        _dbclient->dropIndex(collection.ns().toString(), indexName);
    }

    void MongoClient::createFunction(const std::string &dbName, const MongoFunction &fun, const std::string &existingFunctionName /* = QString() */)
    {
        MongoNamespace ns(dbName, "system.js");
        mongo::BSONObj obj = fun.toBson();

        if (existingFunctionName.empty()) { // this is insert
            _dbclient->insert(ns.toString(), obj);
        } else { // this is update

            std::string name = fun.name();

            if (existingFunctionName == name) {
                mongo::BSONObjBuilder builder;
                builder.append("_id", name);
                mongo::BSONObj bsonQuery = builder.obj();
                mongo::Query query(bsonQuery);

                _dbclient->update(ns.toString(), query, obj, true, false);
            } else {
                _dbclient->insert(ns.toString(), obj);
                std::string res = _dbclient->getLastError();

                // if no errors
                if (res.empty()) {
                    mongo::BSONObjBuilder builder;
                    builder.append("_id", existingFunctionName);
                    mongo::BSONObj bsonQuery = builder.obj();
                    mongo::Query query(bsonQuery);
                    _dbclient->remove(ns.toString(), query, true);
                }
            }
        }
    }

    void MongoClient::dropFunction(const std::string &dbName, const std::string &name)
    {
        MongoNamespace ns(dbName, "system.js");

        mongo::BSONObjBuilder builder;
        builder.append("_id", name);
        mongo::BSONObj bsonQuery = builder.obj();
        mongo::Query query(bsonQuery);

        _dbclient->remove(ns.toString(), query, true);
    }

    void MongoClient::createDatabase(const std::string &dbName)
    {
        /*
        *  Here we are going to insert temp document to "<dbName>.temp" collection.
        *  This will create <dbName> database for us.
        *  Finally we are dropping just created temporary collection.
        */

        MongoNamespace ns(dbName, "temp");

        // If <dbName>.temp already exists, stop.
        if (_dbclient->exists(ns.toString()))
            return;

        // Building { _id : "temp" } document
        mongo::BSONObjBuilder builder;
        builder.append("_id", "temp");
        mongo::BSONObj obj = builder.obj();

        // Insert this document
        _dbclient->insert(ns.toString(), obj);

        // Drop temp collection
        _dbclient->dropCollection(ns.toString());
    }

    void MongoClient::dropDatabase(const std::string &dbName)
    {
        _dbclient->dropDatabase(dbName);
    }

    void MongoClient::createCollection(const MongoNamespace &ns)
    {
        _dbclient->createCollection(ns.toString());
    }

    void MongoClient::renameCollection(const MongoNamespace &ns, const std::string &newCollectionName)
    {
        MongoNamespace from(ns);
        MongoNamespace to(ns.databaseName(), newCollectionName);

        // Building { renameCollection: <source-namespace>, to: <target-namespace> }
        mongo::BSONObjBuilder command; // { collStats: "db.collection", scale : 1 }
        command.append("renameCollection", from.toString());
        command.append("to", to.toString());

        mongo::BSONObj result;
        _dbclient->runCommand("admin", command.obj(), result); // this command should be run against "admin" db
    }

    void MongoClient::duplicateCollection(const MongoNamespace &ns, const std::string &newCollectionName)
    {
        MongoNamespace from(ns);
        MongoNamespace to(ns.databaseName(), newCollectionName);

        if (!_dbclient->exists(to.toString()))
            _dbclient->createCollection(to.toString());

        std::auto_ptr<mongo::DBClientCursor> cursor(_dbclient->query(from.toString(), mongo::Query()));
        while (cursor->more()) {
            mongo::BSONObj bsonObj = cursor->next();
            _dbclient->insert(to.toString(), bsonObj);
        }
    }

    void MongoClient::copyCollectionToDiffServer(mongo::DBClientConnection *const fromServ,const MongoNamespace &from, const MongoNamespace &to)
    {
        if (!_dbclient->exists(to.toString()))
            _dbclient->createCollection(to.toString());

        std::auto_ptr<mongo::DBClientCursor> cursor(fromServ->query(from.toString(), mongo::Query()));
        while (cursor->more()) {
            mongo::BSONObj bsonObj = cursor->next();
            _dbclient->insert(to.toString(), bsonObj);
        }
    }

    void MongoClient::dropCollection(const MongoNamespace &ns)
    {
        _dbclient->dropCollection(ns.toString());
    }

    void MongoClient::insertDocument(const mongo::BSONObj &obj, const MongoNamespace &ns)
    {
        _dbclient->insert(ns.toString(), obj);
    }

    void MongoClient::saveDocument(const mongo::BSONObj &obj, const MongoNamespace &ns)
    {

        mongo::BSONElement id = obj.getField("_id");
        mongo::BSONObjBuilder builder;
        builder.append(id);
        mongo::BSONObj bsonQuery = builder.obj();
        mongo::Query query(bsonQuery);

        _dbclient->update(ns.toString(), query, obj, true, false);
        //_dbclient->save(ns.toString().toStdString(), obj);
    }

    void MongoClient::removeDocuments(const MongoNamespace &ns, mongo::Query query, bool justOne /*= true*/)
    {
        _dbclient->remove(ns.toString(), query, justOne);
    }

    std::vector<MongoDocumentPtr> MongoClient::query(const MongoQueryInfo &info)
    {
        MongoNamespace ns(info._info._ns);

        //int limit = (info.limit <= 0) ? 50 : info.limit;

        std::vector<MongoDocumentPtr> docs;

        if (info._limit == -1) // it means that we do not need to load any documents
            return docs;

        std::auto_ptr<mongo::DBClientCursor> cursor = _dbclient->query(
            ns.toString(), info._query, info._limit, info._skip,
            info._fields.nFields() ? &info._fields : 0, info._options, info._batchSize);

        while (cursor->more()) {
            mongo::BSONObj bsonObj = cursor->next();
            MongoDocumentPtr doc(new MongoDocument(bsonObj.getOwned()));
            docs.push_back(doc);
        }

        return docs;
    }

    MongoCollectionInfo MongoClient::runCollStatsCommand(const std::string &ns)
    {
        MongoNamespace mongons(ns);

        mongo::BSONObjBuilder command; // { collStats: "db.collection", scale : 1 }
        command.append("collStats", mongons.collectionName());
        command.append("scale", 1);

        mongo::BSONObj result;
        _dbclient->runCommand(mongons.databaseName(), command.obj(), result);
        std::string isCV = result.toString();
        MongoCollectionInfo newInfo(result);
        return newInfo;
    }

    std::vector<MongoCollectionInfo> MongoClient::runCollStatsCommand(const std::vector<std::string> &namespaces)
    {
        std::vector<MongoCollectionInfo> infos;
        for (std::vector<std::string>::const_iterator it = namespaces.begin(); it!=namespaces.end(); ++it) {
            MongoCollectionInfo info = runCollStatsCommand(*it);
            if (info.ns().isValid()){
                infos.push_back(info);
            }            
        }
        return infos;
    }

    void MongoClient::done()
    {
        // do nothing here, because we are not using ScopedDbConnection now
        //_scopedConnection->done();
    }
}
