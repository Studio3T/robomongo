#include "robomongo/core/mongodb/MongoClient.h"
#include "robomongo/core/domain/MongoNamespace.h"
#include <mongo/client/dbclient.h>

using namespace Robomongo;
using namespace std;

MongoClient::MongoClient(mongo::ScopedDbConnection *scopedConnection) :
    _scopedConnection(scopedConnection),
    _dbclient(scopedConnection->get()){ }

QStringList MongoClient::getCollectionNames(const QString &dbname)
{
    list<string> dbs = _dbclient->getCollectionNames(dbname.toStdString());

    QStringList stringList;
    for (list<string>::iterator i = dbs.begin(); i != dbs.end(); i++) {
        stringList.append(QString::fromStdString(*i));
    }

    stringList.sort();
    return stringList;
}

QStringList MongoClient::getDatabaseNames()
{
    list<string> dbs = _dbclient->getDatabaseNames();

    QStringList dbNames;
    for (list<string>::iterator i = dbs.begin(); i != dbs.end(); ++i) {
        dbNames.append(QString::fromStdString(*i));
    }

    dbNames.sort();
    return dbNames;
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
    auto_ptr<mongo::DBClientCursor> cursor = _dbclient->query(
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
    foreach (QString ns, namespaces) {
        MongoCollectionInfo info = runCollStatsCommand(ns);
        infos.append(info);
    }
    return infos;
}

void MongoClient::done()
{
    _scopedConnection->done();
}
