#include "robomongo/core/mongodb/MongoClient.h"

#include "robomongo/core/utils/BsonUtils.h"

namespace
{
    Robomongo::EnsureIndexInfo makeEnsureIndexInfoFromBsonObj(const Robomongo::MongoCollectionInfo &collection,const mongo::BSONObj &obj)
    {
        using namespace Robomongo::BsonUtils;
        Robomongo::EnsureIndexInfo info(collection);
        std::string str = obj.toString();
        info._name = getField<mongo::String>(obj,"name");
        mongo::BSONObj keyObj = getField<mongo::Object>(obj,"key");
        if(keyObj.isValid()){
            std::string key = keyObj.toString();
            if(!key.empty()){
                info._request = key.substr(5);
            }
        }
        info._unique = getField<mongo::Bool>(obj,"unique");
        info._backGround = getField<mongo::Bool>(obj,"background");
        info._dropDups = getField<mongo::Bool>(obj,"dropDups"); 
        info._sparse = getField<mongo::Bool>(obj,"sparse");
        info._ttl = getField<mongo::NumberInt>(obj,"expireAfterSeconds"); 
        info._defaultLanguage = getField<mongo::String>(obj,"default_language");
        info._languageOverride = getField<mongo::String>(obj,"language_override");
        info._textWeights = getField<mongo::String>(obj,"weights");               
        return info;
    }
}

namespace Robomongo
{
    MongoClient::MongoClient(mongo::DBClientBase *dbclient) :
        _dbclient(dbclient) { }

    QStringList MongoClient::getCollectionNames(const QString &dbname) const
    {
        typedef std::list<std::string> cont_string_t;
        cont_string_t dbs = _dbclient->getCollectionNames(dbname.toStdString());

        QStringList stringList;
        for (cont_string_t::const_iterator i = dbs.begin(); i != dbs.end(); i++) {
            stringList.append(QString::fromStdString(*i));
        }

        stringList.sort();
        return stringList;
    }

    QStringList MongoClient::getDatabaseNames() const
    {
        typedef std::list<std::string> cont_string_t;
        cont_string_t dbs = _dbclient->getDatabaseNames();
        QStringList dbNames;
        for (cont_string_t::const_iterator i = dbs.begin(); i != dbs.end(); ++i)
        {
            dbNames.append(QString::fromStdString(*i));
        }
        dbNames.sort();
        return dbNames;
    }

    QList<MongoUser> MongoClient::getUsers(const QString &dbName)
    {
        MongoNamespace ns(dbName, "system.users");
        QList<MongoUser> users;

        std::auto_ptr<mongo::DBClientCursor> cursor(_dbclient->query(ns.toString().toStdString(), mongo::Query()));

        while (cursor->more()) {
            mongo::BSONObj bsonObj = cursor->next();
            MongoUser user(bsonObj);
            users.append(user);
        }

        return users;
    }

    void MongoClient::createUser(const QString &dbName, const MongoUser &user, bool overwrite)
    {
        MongoNamespace ns(dbName, "system.users");
        mongo::BSONObj obj = user.toBson();

        if (!overwrite) {
            _dbclient->insert(ns.toString().toStdString(), obj);
        } else {
            mongo::BSONElement id = obj.getField("_id");
            mongo::BSONObjBuilder builder;
            builder.append(id);
            mongo::BSONObj bsonQuery = builder.obj();
            mongo::Query query(bsonQuery);

            _dbclient->update(ns.toString().toStdString(), query, obj, true, false);
        }
    }

    void MongoClient::dropUser(const QString &dbName, const mongo::OID &id)
    {
        MongoNamespace ns(dbName, "system.users");

        mongo::BSONObjBuilder builder;
        builder.append("_id", id);
        mongo::BSONObj bsonQuery = builder.obj();
        mongo::Query query(bsonQuery);

        _dbclient->remove(ns.toString().toStdString(), query, true);
    }

    QList<MongoFunction> MongoClient::getFunctions(const QString &dbName)
    {
        MongoNamespace ns(dbName, "system.js");
        QList<MongoFunction> functions;

        std::auto_ptr<mongo::DBClientCursor> cursor(_dbclient->query(ns.toString().toStdString(), mongo::Query()));

        while (cursor->more()) {
            mongo::BSONObj bsonObj = cursor->next();

            try {
                MongoFunction user(bsonObj);
                functions.append(user);
            } catch (const std::exception &) {
            // skip invalid docs
            }
        }
        return functions;
    }

    QList<EnsureIndexInfo> MongoClient::getIndexes(const MongoCollectionInfo &collection) const
    {
        QList<EnsureIndexInfo> result;
        std::auto_ptr<mongo::DBClientCursor> cursor(_dbclient->getIndexes(collection.ns().toString().toStdString()));

        while (cursor->more()) {
            mongo::BSONObj bsonObj = cursor->next();
            result.push_back(makeEnsureIndexInfoFromBsonObj(collection,bsonObj));
        }

        return result;
    }

    void MongoClient::ensureIndex(const EnsureIndexInfo &oldInfo,const EnsureIndexInfo &newInfo) const
    {   
        std::string ns = newInfo._collection.ns().toString().toStdString();
        mongo::BSONObj keys = mongo::fromjson(newInfo._request);
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

        if( version >= 0 ) 
            toSave.append("v", version);

        if(oldInfo._unique!=newInfo._unique)
            toSave.appendBool( "unique", newInfo._unique );

        if(oldInfo._backGround!=newInfo._backGround)
            toSave.appendBool( "background", newInfo._backGround );

        if(oldInfo._dropDups!=newInfo._dropDups)
            toSave.appendBool("dropDups",newInfo._dropDups);

        if(oldInfo._sparse!=newInfo._sparse)
            toSave.appendBool("sparse",newInfo._sparse);

        if(oldInfo._defaultLanguage!=newInfo._defaultLanguage)
            toSave.append("default_language",newInfo._defaultLanguage);

        if(oldInfo._languageOverride!=newInfo._languageOverride)
            toSave.append("language_override",newInfo._languageOverride);

        if(oldInfo._textWeights!=newInfo._textWeights)
            toSave.append("weights",newInfo._textWeights);

       /* if ( _seenIndexes.count( cacheKey ) )
            return 0;

        if ( cache )
            _seenIndexes.insert( cacheKey );*/

        if (oldInfo._ttl!=newInfo._ttl)
            toSave.append( "expireAfterSeconds", newInfo._ttl );

        MongoNamespace namesp(newInfo._collection.ns().databaseName(), "system.indexes");
        mongo::BSONObj obj = toSave.obj();
        if(!oldInfo._name.empty())
            _dbclient->dropIndex(ns, oldInfo._name);
        _dbclient->insert(  namesp.toString().toStdString().c_str() , obj ); 
    }

    void MongoClient::renameIndexFromCollection(const MongoCollectionInfo &collection, const QString &oldIndexName, const QString &newIndexName) const
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
        std::string systemIndexesNs = ns.toString().toStdString();
        std::string oldIndexNameStd = oldIndexName.toStdString();

        // Building this JSON: { "name" : "oldIndexName" }
        mongo::BSONObj query(mongo::BSONObjBuilder()
            .append("name", oldIndexNameStd)
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
        std::string newIndexNameStd = newIndexName.toStdString();
        while (i.more()) {
            mongo::BSONElement element = i.next();

            if (mongo::StringData(element.fieldName()).compare("name") == 0) {
                builder.append("name", newIndexNameStd);
                continue;
            }

            builder.append(element);
        }
        std::string collectionNs = collection.ns().toString().toStdString();

        _dbclient->dropIndex(collectionNs, oldIndexNameStd);
        _dbclient->insert(systemIndexesNs, builder.obj());
    }

    void MongoClient::dropIndexFromCollection(const MongoCollectionInfo &collection, const QString &indexName)const
    {
        _dbclient->dropIndex(collection.ns().toString().toStdString(), indexName.toStdString());
    }

    void MongoClient::createFunction(const QString &dbName, const MongoFunction &fun, const QString &existingFunctionName /* = QString() */)
    {
        MongoNamespace ns(dbName, "system.js");
        mongo::BSONObj obj = fun.toBson();

        if (existingFunctionName.isEmpty()) { // this is insert
            _dbclient->insert(ns.toString().toStdString(), obj);
        } else { // this is update

        QString name = fun.name();

        if (existingFunctionName == name) {
            mongo::BSONObjBuilder builder;
            builder.append("_id", name.toStdString());
            mongo::BSONObj bsonQuery = builder.obj();
            mongo::Query query(bsonQuery);

            _dbclient->update(ns.toString().toStdString(), query, obj, true, false);
        } else {
            _dbclient->insert(ns.toString().toStdString(), obj);
            std::string res = _dbclient->getLastError();

                // if no errors
                if (res.empty()) {
                    mongo::BSONObjBuilder builder;
                    builder.append("_id", existingFunctionName.toStdString());
                    mongo::BSONObj bsonQuery = builder.obj();
                    mongo::Query query(bsonQuery);
                    _dbclient->remove(ns.toString().toStdString(), query, true);
                }
            }
        }
    }

    void MongoClient::dropFunction(const QString &dbName, const QString &name)
    {
        MongoNamespace ns(dbName, "system.js");

        mongo::BSONObjBuilder builder;
        builder.append("_id", name.toStdString());
        mongo::BSONObj bsonQuery = builder.obj();
        mongo::Query query(bsonQuery);

        _dbclient->remove(ns.toString().toStdString(), query, true);
    }

    void MongoClient::createDatabase(const QString &dbName)
    {
        /*
        *  Here we are going to insert temp document to "<dbName>.temp" collection.
        *  This will create <dbName> database for us.
        *  Finally we are dropping just created temporary collection.
        */

        MongoNamespace ns(dbName, "temp");

        // If <dbName>.temp already exists, stop.
        if (_dbclient->exists(ns.toString().toStdString()))
            return;

        // Building { _id : "temp" } document
        mongo::BSONObjBuilder builder;
        builder.append("_id", "temp");
        mongo::BSONObj obj = builder.obj();

        // Insert this document
        _dbclient->insert(ns.toString().toStdString(), obj);

        // Drop temp collection
        _dbclient->dropCollection(ns.toString().toStdString());
    }

    void MongoClient::dropDatabase(const QString &dbName)
    {
        _dbclient->dropDatabase(dbName.toStdString());
    }

    void MongoClient::createCollection(const QString &dbName, const QString &collectionName)
    {
        MongoNamespace ns(dbName, collectionName);
        _dbclient->createCollection(ns.toString().toStdString());
    }

    void MongoClient::renameCollection(const QString &dbName, const QString &collectionName, const QString &newCollectionName)
    {
        MongoNamespace from(dbName, collectionName);
        MongoNamespace to(dbName, newCollectionName);

        // Building { renameCollection: <source-namespace>, to: <target-namespace> }
        mongo::BSONObjBuilder command; // { collStats: "db.collection", scale : 1 }
        command.append("renameCollection", from.toString().toStdString());
        command.append("to", to.toString().toStdString());

        mongo::BSONObj result;
        _dbclient->runCommand("admin", command.obj(), result); // this command should be run against "admin" db
    }

    void MongoClient::duplicateCollection(const QString &dbName, const QString &collectionName, const QString &newCollectionName)
    {
        MongoNamespace from(dbName, collectionName);
        MongoNamespace to(dbName, newCollectionName);

        std::auto_ptr<mongo::DBClientCursor> cursor(_dbclient->query(from.toString().toStdString(), mongo::Query()));
        while (cursor->more()) {
            mongo::BSONObj bsonObj = cursor->next();
            _dbclient->insert(to.toString().toStdString(), bsonObj);
        }
    }

    void MongoClient::dropCollection(const QString &dbName, const QString &collectionName)
    {
        MongoNamespace ns(dbName, collectionName);
        _dbclient->dropCollection(ns.toString().toStdString());
    }

    void MongoClient::insertDocument(const mongo::BSONObj &obj, const QString &db, const QString &collection)
    {
        MongoNamespace ns(db, collection);
        _dbclient->insert(ns.toString().toStdString(), obj);
    }

    void MongoClient::saveDocument(const mongo::BSONObj &obj, const QString &db, const QString &collection)
    {
        MongoNamespace ns(db, collection);

        mongo::BSONElement id = obj.getField("_id");
        mongo::BSONObjBuilder builder;
        builder.append(id);
        mongo::BSONObj bsonQuery = builder.obj();
        mongo::Query query(bsonQuery);

        _dbclient->update(ns.toString().toStdString(), query, obj, true, false);
        //_dbclient->save(ns.toString().toStdString(), obj);
    }

    void MongoClient::removeDocuments(const QString &db, const QString &collection, mongo::Query query, bool justOne /*= true*/)
    {
        MongoNamespace ns(db, collection);
        _dbclient->remove(ns.toString().toStdString(), query, justOne);
    }

    QList<MongoDocumentPtr> MongoClient::query(const MongoQueryInfo &info)
    {
        MongoNamespace ns(info.databaseName, info.collectionName);

        int limit = (info.limit == 0 || info.limit > 51) ? 50 : info.limit;

        QList<MongoDocumentPtr> docs;
        std::auto_ptr<mongo::DBClientCursor> cursor = _dbclient->query(
            ns.toString().toStdString(), info.query, limit, info.skip,
            info.fields.nFields() ? &info.fields : 0, info.options, info.batchSize);

        while (cursor->more()) {
            mongo::BSONObj bsonObj = cursor->next();
            MongoDocumentPtr doc(new MongoDocument(bsonObj.getOwned()));
            docs.append(doc);
        }

        return docs;
    }

    MongoCollectionInfo MongoClient::runCollStatsCommand(const QString &ns)
    {
        MongoNamespace mongons(ns);

        mongo::BSONObjBuilder command; // { collStats: "db.collection", scale : 1 }
        command.append("collStats", mongons.collectionName().toStdString());
        command.append("scale", 1);

        mongo::BSONObj result;
        _dbclient->runCommand(mongons.databaseName().toStdString(), command.obj(), result);

        MongoCollectionInfo newInfo(result);
        return newInfo;
    }

    QList<MongoCollectionInfo> MongoClient::runCollStatsCommand(const QStringList &namespaces)
    {
        QList<MongoCollectionInfo> infos;
        for(QStringList::const_iterator it=namespaces.begin();it!=namespaces.end();++it){
            MongoCollectionInfo info = runCollStatsCommand(*it);
            infos.append(info);
        }
        return infos;
    }

    void MongoClient::done()
    {
        // do nothing here, because we are not using ScopedDbConnection now
        //_scopedConnection->done();
    }
}
