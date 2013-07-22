#include "robomongo/core/mongodb/MongoClient.h"

namespace
{
    namespace detail
    {
        template<mongo::BSONType BSONType_t>
        struct func_traits
        {
            typedef std::string type;
            static type getField(const mongo::BSONElement &elem)
            {
                return elem.toString();
            }
        };
        template<>
        struct func_traits<mongo::Bool>
        {
            typedef bool type;
            static type getField(const mongo::BSONElement &elem)
            {
                return elem.Bool();
            }
        };
        template<>
        struct func_traits<mongo::String>
        {
            typedef std::string type;
            static type getField(const mongo::BSONElement &elem)
            {
                return elem.String();
            }
        };
        template<>
        struct func_traits<mongo::NumberInt>
        {
            typedef int type;
            static type getField(const mongo::BSONElement &elem)
            {
                return elem.Int();
            }
        };
    }

    template<mongo::BSONType BSONType_t>
    typename detail::func_traits<BSONType_t>::type getField(const mongo::BSONObj &obj,const char *data)
    {
        typedef typename detail::func_traits<BSONType_t> func_t;
        func_t::type result=func_t::type();
        mongo::BSONElement elem = obj.getField(data);
        if (!elem.eoo()){
            result = func_t::getField(elem);
        }  
        return result;
    }

    Robomongo::EnsureIndexInfo makeEnsureIndexInfoFromBsonObj(const Robomongo::MongoCollectionInfo &collection,const mongo::BSONObj &obj)
    {
        Robomongo::EnsureIndexInfo info(collection);
        info._name = QString::fromUtf8(getField<mongo::String>(obj,"name").c_str());
        const char* key = getField<mongo::Object>(obj,"key").c_str();
        if(key){
            info._request = QString::fromUtf8(key+5);
        }
        info._isUnique = getField<mongo::Bool>(obj,"unique");
        info._isBackGround = getField<mongo::Bool>(obj,"background");
        info._isDropDuplicates = getField<mongo::Bool>(obj,"dropDups"); 
        info._isSparce = getField<mongo::Bool>(obj,"sparse");
        info._expireAfter = getField<mongo::NumberInt>(obj,"expireAfterSeconds"); 
        info._defaultLanguage = QString::fromUtf8(getField<mongo::String>(obj,"default_language").c_str());
        info._languageOverride = QString::fromUtf8(getField<mongo::String>(obj,"language_override").c_str());
        info._textWeights = QString::fromUtf8(getField<mongo::String>(obj,"weights").c_str());               
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

    void MongoClient::ensureIndex(const MongoCollectionInfo &collection, const QString &name, const QString &request, bool unique, bool backGround, bool dropDuplicates,
        bool sparse,int expireAfter,const QString &defaultLanguage,const QString &languageOverride,const QString &textWeights) const
    {
        // TODO: This function should work for creating and editing of indexes.
        // If index with "name" already exists - drop and create new.
        // If index with "name doesn't exist - simply create new.
        //
        // In this case we do not need MongoClient::renameIndexFromCollection(), because
        // we will use MongoClient::ensureIndex() even for name changing of Index.
        // But let's leave MongoClient::renameIndexFromCollection() for future references.

        mongo::BSONObj obj = mongo::fromjson(request.toUtf8());
       // builder.append("dropDups",dropDuplicates);
       // builder.appendBool("sparse",sparse);
      //  builder.append("default_language",defaultLanguage.toStdString());
       // builder.append("language_override",languageOverride.toStdString());
       // builder.append("weights",textWeights.toStdString());
        _dbclient->ensureIndex(collection.ns().toString().toStdString(), obj, unique, name.toStdString(), true, backGround, -1, expireAfter);
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

        MongoCollectionInfo info(result);
        return info;
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
