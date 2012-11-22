#include "MongoDatabase.h"
#include "MongoServer.h"
#include "MongoCollection.h"

using namespace Robomongo;

MongoDatabase::MongoDatabase(const MongoServer *server, const QString &name) : QObject(),
    _system(false)
{
    _server = server;
    _name = name;

    // Check that this is a system database
    _system = name == "admin" ||
              name == "local";

    const MongoClient *client = _server->client();

    connect(client, SIGNAL(collectionNamesLoaded(QString, QStringList)),
            this,   SLOT(collectionNamesLoaded(QString, QStringList)));
}

MongoDatabase::~MongoDatabase()
{
    int dta = 87;
}

void MongoDatabase::listCollections()
{
    _server->client()->loadCollectionNames(this, _name);
}

bool MongoDatabase::event(QEvent *event)
{
    CollectionNamesLoaded *loaded = static_cast<CollectionNamesLoaded *>(event);

    QList<MongoCollectionPtr> list;

    foreach(QString name, loaded->_collectionNames)    {
        MongoCollectionPtr db(new MongoCollection(this, name));
        list.append(db);
    }

    emit collectionListLoaded(list);
}


void MongoDatabase::collectionNamesLoaded(const QString &databaseName, const QStringList &collectionNames)
{

}
