#include "robomongo/core/mongodb/MongoClient.h"

#include "mongo/db/namespace_string.h"

#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/shell/bson/json.h"

namespace
{
    Robomongo::EnsureIndexInfo makeEnsureIndexInfoFromBsonObj(
        const Robomongo::MongoCollectionInfo &collection,
        const mongo::BSONObj &obj)
    {
        using namespace Robomongo::BsonUtils;
        Robomongo::EnsureIndexInfo info(collection);
        info._name = obj.getStringField("name");
        mongo::BSONObj keyObj = obj.getObjectField("key");
        if (keyObj.isValid()) 
            info._keys = jsonString(keyObj, mongo::TenGen, 1, Robomongo::DefaultEncoding, Robomongo::Utc);

        info._unique = obj.getBoolField("unique");
        info._backGround = obj.getBoolField("background");
        info._sparse = obj.getBoolField("sparse");
        info._ttl = obj.getIntField("expireAfterSeconds");
        info._defaultLanguage = obj.getStringField("default_language");
        info._languageOverride = obj.getStringField("language_override");
        mongo::BSONObj weightsObj = obj.getObjectField("weights");
        if (weightsObj.isValid()) 
            info._textWeights = jsonString(weightsObj, mongo::TenGen, 1, Robomongo::DefaultEncoding, 
                                           Robomongo::Utc);

        return info;
    }
}

namespace Robomongo
{
    MongoClient::MongoClient(mongo::DBClientBase *const dbclient) :
        _dbclient(dbclient) { }

    std::vector<std::string> MongoClient::getCollectionNamesWithDbname(const std::string &dbname) const
    {
        std::list<mongo::BSONObj> collList = _dbclient->getCollectionInfos(dbname);

        std::vector<std::string> collNames;	
        for (auto const& coll : collList)
            collNames.push_back(dbname + '.' + coll.getStringField("name")); // todo: verify

        std::sort(collNames.begin(), collNames.end());
        return collNames;
    }

    // Warning: 
    // Use string version dbVersionStr(), version number is corrupted after conversion to float
    // Todo: Remove this function
    float MongoClient::getVersion() const
    {
        float result = 0.0f;
        mongo::BSONObj resultObj;
        _dbclient->runCommand("db", BSON("buildInfo" << "1"), resultObj);
        std::string resultStr = BsonUtils::getField<mongo::String>(resultObj, "version");
        result = atof(resultStr.c_str());
        return result;
    }

    std::string MongoClient::dbVersionStr() const
    {
        mongo::BSONObj resultObj;
        _dbclient->runCommand("db", BSON("buildInfo" << "1"), resultObj);
        std::string const resultStr = BsonUtils::getField<mongo::String>(resultObj, "version");
        return resultStr;
    }

    std::string MongoClient::getStorageEngineType() const
    {
        mongo::BSONObj resultObj;
        _dbclient->runCommand("db", BSON("serverStatus" << "1"), resultObj);
        return resultObj.getObjectField("storageEngine").getStringField("name");
    }

    std::vector<std::string> MongoClient::getDatabaseNames() const
    {
        std::list<std::string> const& dbs = _dbclient->getDatabaseNames();
        std::vector<std::string> dbNames = {dbs.begin(), dbs.end()};
        std::sort(dbNames.begin(), dbNames.end());
        return dbNames;
    }

    std::vector<MongoUser> MongoClient::getUsers(const std::string &dbName)
    {
        mongo::BSONObjBuilder cmd;
        cmd.append("usersInfo", 1);

        mongo::BSONObj result;
        if (!_dbclient->runCommand(dbName, cmd.done(), result)) {
            std::string errStr = result.getStringField("errmsg");
            if (errStr.empty())
                errStr = "Failed to get error message.";

            throw std::runtime_error(errStr);
        }

        std::vector<MongoUser> users;
        for (auto const& usr : result.getField("users").Array())
            users.push_back(MongoUser(getVersion(), usr.embeddedObject()));

        return users;
    }

    void MongoClient::createUser(const std::string &dbName, const MongoUser &user)
    {
        mongo::BSONObjBuilder cmd;
        cmd.append("createUser", user.name());
        cmd.append("pwd", user.password());
        
        mongo::BSONArrayBuilder roles;
        auto const& rolesStrs = user.roles();
        for (auto const& roleStr : rolesStrs) {
            mongo::BSONObjBuilder role;
            role.append("role", roleStr).append("db", user.userSource());
            roles.append(role.done());
        }
        cmd.appendArray("roles", roles.done());

        mongo::BSONObj result;
        if (!_dbclient->runCommand(dbName, cmd.done(), result)) {
            std::string errStr = result.getStringField("errmsg");
            if (errStr.empty())
                errStr = "Failed to get error message.";
   
            throw std::runtime_error(errStr);
        }       
    }

    void MongoClient::dropUser(const std::string &dbName, const std::string &user)
    {
        mongo::BSONObjBuilder cmd;
        cmd.append("dropUser", user);

        mongo::BSONObj result;
        if (!_dbclient->runCommand(dbName, cmd.done(), result)) {
            std::string errStr = result.getStringField("errmsg");
            if (errStr.empty())
                errStr = "Failed to get error message.";

            throw std::runtime_error(errStr);
        }
    }

    std::vector<MongoFunction> MongoClient::getFunctions(const std::string &dbName) const
    {
        std::vector<MongoFunction> functions;

        std::unique_ptr<mongo::DBClientCursor> cursor(
            _dbclient->query(mongo::NamespaceString(dbName, "system.js"), mongo::Query().sort("_id")));

        // Cursor may be NULL, it means we have connectivity problem
        if (!cursor)
            throw std::runtime_error("Network error while attempting to load list of functions.");

        while (cursor->more()) {
            mongo::BSONObj bsonObj = cursor->next();
            try {
                MongoFunction func(bsonObj);
                functions.push_back(func);
            } catch (const std::exception &) {
                // skip invalid docs
            }
        }
        return functions;
    }

    std::vector<EnsureIndexInfo> MongoClient::getIndexes(const MongoCollectionInfo &collection) const
    {
        std::vector<EnsureIndexInfo> result;
        std::list<mongo::BSONObj> indexes = _dbclient->getIndexSpecs(collection.ns().toString());

        for (std::list<mongo::BSONObj>::iterator it = indexes.begin(); it != indexes.end(); ++it) {
            mongo::BSONObj bsonObj = *it;
            result.push_back(makeEnsureIndexInfoFromBsonObj(collection, bsonObj));
        }

        return result;
    }

    void MongoClient::ensureIndex(const EnsureIndexInfo &oldInfo, const EnsureIndexInfo &newInfo) const
    {   
        mongo::IndexSpec indexSpec;
        indexSpec.name(newInfo._name);
        indexSpec.addKeys(mongo::Robomongo::fromjson(newInfo._keys));

        mongo::BSONObjBuilder optionsBuilder;
        
        auto const addIfTrue = [&](auto const& keyValuePair) {  
            if (keyValuePair.second)
                optionsBuilder.appendBool(keyValuePair.first, true);
        };        

        addIfTrue(std::pair{ "unique", newInfo._unique });
        addIfTrue(std::pair{ "background", newInfo._backGround });
        addIfTrue(std::pair{ "sparse", newInfo._sparse });

        if (!newInfo._defaultLanguage.empty())
            optionsBuilder.append("default_language", newInfo._defaultLanguage);

        if (!newInfo._languageOverride.empty())
            optionsBuilder.append("language_override", newInfo._languageOverride);
        
        if (!mongo::Robomongo::fromjson(newInfo._textWeights).isEmpty())
            optionsBuilder.append("weights", mongo::Robomongo::fromjson(newInfo._textWeights));

        if (newInfo._ttl > 0)
            optionsBuilder.append("expireAfterSeconds", newInfo._ttl);

        indexSpec.addOptions(optionsBuilder.obj());

        std::string const ns = newInfo._collection.ns().toString();
        if (!oldInfo._name.empty())
            _dbclient->dropIndex(ns, oldInfo._name);

        _dbclient->createIndex(ns, indexSpec);

        std::string const errorStr = _dbclient->getLastError();
        if (!errorStr.empty())
            throw std::runtime_error(errorStr);
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

    void MongoClient::createFunction(const std::string &dbName, const MongoFunction &fun, 
                                     const std::string &existingFunctionName /* = QString() */)
    {
        MongoNamespace ns(dbName, "system.js");
        mongo::BSONObj obj = fun.toBson();

        if (existingFunctionName.empty()) { // create new function
            _dbclient->insert(ns.toString(), obj);
            std::string errorStr = _dbclient->getLastError();
            if (!errorStr.empty())
                throw std::runtime_error(errorStr/* , 0 */);
        } else { // this is update

            std::string name = fun.name();

            if (existingFunctionName == name) { // update existing function code
                mongo::BSONObjBuilder builder;
                builder.append("_id", name);
                mongo::BSONObj bsonQuery = builder.obj();
                mongo::Query query(bsonQuery);

                _dbclient->update(ns.toString(), query, obj, true, false);
                std::string errorStr = _dbclient->getLastError();
                if (!errorStr.empty())
                    throw std::runtime_error(errorStr);
            } else {    // update function name (remove & insert)
                _dbclient->insert(ns.toString(), obj);
                std::string errorStr = _dbclient->getLastError();

                // if no errors
                if (errorStr.empty()) {
                    mongo::BSONObjBuilder builder;
                    builder.append("_id", existingFunctionName);
                    mongo::BSONObj bsonQuery = builder.obj();
                    mongo::Query query(bsonQuery);
                    _dbclient->remove(ns.toString(), query, true);
                }
                else {
                    throw std::runtime_error(errorStr);
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
        std::string errorStr = _dbclient->getLastError();
        if (!errorStr.empty())
            throw std::runtime_error(errorStr);
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
            throw std::runtime_error(dbName + ".temp already exists.");

        // Building { _id : "temp" } document
        mongo::BSONObjBuilder builder;
        builder.append("_id", "temp");
        mongo::BSONObj obj = builder.obj();

        // Insert this document
        _dbclient->insert(ns.toString(), obj);
        std::string errorStr = _dbclient->getLastError();
        if (!errorStr.empty())
            throw std::runtime_error(errorStr);

        // Drop temp collection
        _dbclient->dropCollection(ns.toString());
    }

    void MongoClient::dropDatabase(const std::string &dbName)
    {
        mongo::BSONObj info;
        if (!_dbclient->dropDatabase(dbName, mongo::WriteConcernOptions(), &info)) { // todo: do we catch errorStr via info - test it??
            std::string errStr = info.toString();
            if (errStr.empty())
                errStr = "Failed to get error message.";

            throw std::runtime_error(errStr);
        }
    }

    void MongoClient::createCollection(const std::string& ns, long long size, bool capped, int max, 
                                       const mongo::BSONObj& extraOptions, mongo::BSONObj* info)
    {
        verify(!capped || size);
        mongo::BSONObj o;
        if (info == 0)
            info = &o;
        mongo::BSONObjBuilder b;
        std::string db = mongo::nsToDatabase(ns);
        b.append("create", ns.c_str() + db.length() + 1);
        if (size) {
            b.append("size", size);
        }
        if (capped) {
            b.append("capped", true);
        }
        if (max) {
            b.append("max", max);
        }
        b.appendElements(extraOptions);

        if (!_dbclient->exists(ns)) {
            mongo::BSONObj result;
            if (!_dbclient->runCommand(db.c_str(), b.done(), result)) {
                std::string errStr = result.getStringField("errmsg");
                if (errStr.empty())
                    errStr = "Failed to get error message.";

                throw std::runtime_error(errStr);
            }
        }
        else {
            throw std::runtime_error("Collection with same name already exists.");
        }
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
        if (!_dbclient->runCommand("admin", command.obj(), result)) { // this command should be run against "admin" db
            std::string errStr = result.getStringField("errmsg");
            if (errStr.empty())
                errStr = "Failed to get error message.";

            throw std::runtime_error(errStr);
        }
    }

    void MongoClient::duplicateCollection(const MongoNamespace &ns, const std::string &newCollectionName)
    {
        MongoNamespace const newCollection(ns.databaseName(), newCollectionName);

        if (!_dbclient->exists(newCollection.toString())) {
            mongo::BSONObj result;
            // todo: Issue #1258 : Duplicate Collection should support advanced collection options.
            //       _dbclient->createCollection() should be called with properties of source collection
            //       not with default parameters as below.
            if (!_dbclient->createCollection(newCollection.toString(), 0, false, 0, &result)) {
                std::string errStr = result.getStringField("errmsg");
                if (errStr.empty())
                    errStr = "Failed to get error message.";

                throw std::runtime_error(errStr);
            }
        }
        else {
            throw std::runtime_error("Collection with same name already exists.");
        }

        std::unique_ptr<mongo::DBClientCursor> cursor {
			_dbclient->query(mongo::NamespaceString(ns.databaseName(), ns.collectionName()), mongo::Query()) 
		};

        // Cursor may be NULL, it means we have connectivity problem
        if (!cursor)
            throw std::runtime_error("Network error while attempting to run query");

        while (cursor->more()) {
            mongo::BSONObj bsonObj = cursor->next();
            _dbclient->insert(newCollection.toString(), bsonObj);
        }
    }

    void MongoClient::copyCollectionToDiffServer(mongo::DBClientBase *const fromServ, const MongoNamespace &from, 
                                                 const MongoNamespace &to)
    {
        if (!_dbclient->exists(to.toString()))
            _dbclient->createCollection(to.toString());

        std::unique_ptr<mongo::DBClientCursor> cursor{fromServ->query(
            mongo::NamespaceString(from.databaseName(), from.collectionName()),
            mongo::Query()) 
		};

        // Cursor may be NULL, it means we have connectivity problem
        if (!cursor)
            throw std::runtime_error("Network error while attempting to run query");

        while (cursor->more()) {
            mongo::BSONObj bsonObj = cursor->next();
            _dbclient->insert(to.toString(), bsonObj);
        }
    }

    void MongoClient::dropCollection(const MongoNamespace &ns)
    {
        if (_dbclient->exists(ns.toString())) {
            mongo::BSONObj info;
            if (!_dbclient->dropCollection(ns.toString(), mongo::WriteConcernOptions(), &info)) { 
                std::string errStr = info.toString();
                if (errStr.empty())
                    errStr = "Failed to get error message.";

                throw std::runtime_error(errStr);
            }
        }
        else {
            throw std::runtime_error("Collection does not exist.");
        }
    }

    void MongoClient::insertDocument(const mongo::BSONObj &obj, const MongoNamespace &ns)
    {
        _dbclient->insert(ns.toString(), obj);
        checkLastErrorAndThrow(ns.databaseName());
    }

    void MongoClient::saveDocument(const mongo::BSONObj &obj, const MongoNamespace &ns)
    {
        mongo::BSONElement id = obj.getField("_id");
        mongo::BSONObjBuilder builder;
        builder.append(id);
        mongo::BSONObj bsonQuery = builder.obj();
        mongo::Query query(bsonQuery);

        _dbclient->update(ns.toString(), query, obj, true, false);
        checkLastErrorAndThrow(ns.databaseName());
    }

    void MongoClient::removeDocuments(const MongoNamespace &ns, mongo::Query query, bool justOne /*= true*/)
    {
        _dbclient->remove(ns.toString(), query, justOne);        
        checkLastErrorAndThrow(ns.databaseName());
    }

    std::vector<MongoDocumentPtr> MongoClient::query(const MongoQueryInfo &info)
    {
        MongoNamespace ns(info._info._ns);

        //int limit = (info.limit <= 0) ? 50 : info.limit;

        std::vector<MongoDocumentPtr> docs;

        if (info._limit == -1) // it means that we do not need to load any documents
            return docs;

        std::unique_ptr<mongo::DBClientCursor> cursor = _dbclient->query(
			mongo::NamespaceString(ns.databaseName(), ns.collectionName()),          
			info._query, info._limit, info._skip, info._fields.nFields() ? &info._fields : 0, 
			info._options, info._batchSize
		);

        // DBClientBase::query may return nullptr
        if (!cursor)
            throw std::runtime_error("Network error while attempting to run query");

        while (cursor->more()) {
            mongo::BSONObj bsonObj = cursor->next();
            MongoDocumentPtr doc(new MongoDocument(bsonObj.getOwned()));
            docs.push_back(doc);
        }

        return docs;
    }

    MongoCollectionInfo MongoClient::runCollStatsCommand(const std::string &ns)
    {
        MongoCollectionInfo info(ns);
        return info;

/*      // Commented for now, to speedup load of collection names
        MongoNamespace mongons(ns);

        mongo::BSONObjBuilder command; // { collStats: "db.collection", scale : 1 }
        command.append("collStats", mongons.collectionName());
        command.append("scale", 1);

        mongo::BSONObj result;
        _dbclient->runCommand(mongons.databaseName(), command.obj(), result);
        std::string isCV = result.toString();
        MongoCollectionInfo newInfo(result);
        return newInfo;
        */
    }

    std::vector<MongoCollectionInfo> MongoClient::runCollStatsCommand(const std::vector<std::string> &namespaces)
    {
        std::vector<MongoCollectionInfo> infos;
        for (auto const& ns : namespaces) {
            MongoCollectionInfo info = runCollStatsCommand(ns);
            if (info.ns().isValid()) 
                infos.push_back(info);
        }
        return infos;
    }

    void MongoClient::done()
    {
        // do nothing here, because we are not using ScopedDbConnection now
        //_scopedConnection->done();
    }

    void MongoClient::checkLastErrorAndThrow(const std::string &db)
    {
        std::string const lastError = _dbclient->getLastError(db);        
        if (lastError.empty())
            return;

        throw std::runtime_error(lastError/*, mongo::ErrorCodes::InternalError*/);
    }
}
