#include "MongoClient.h"
#include <QThread>
#include <QStringList>
#include "boost/scoped_ptr.hpp"
#include "mongo/client/dbclient.h"
#include <QMutexLocker>
#include <QCoreApplication>
#include "events/MongoEvents.h"
#include <QFile>
#include <QTextStream>

using namespace Robomongo;
using namespace std;
using namespace mongo;

MongoClient::MongoClient(QString address, QObject *parent) : QObject(parent)
{
    _address = address;
    init();
}


MongoClient::MongoClient(QString host, int port, QObject *parent) : QObject(parent)
{
    _address = QString("%1:%2").arg(host).arg(port);
    init();
}

MongoClient::~MongoClient()
{
    _thread->quit();
    _thread->wait(1000);

    delete _thread;
}

/**
 * @brief Initialise MongoClient
 */
void MongoClient::init()
{
    _thread = new QThread();
    this->moveToThread(_thread);

    _thread->start();
}

void MongoClient::evaluteFile(const QString &path)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Unable to read " + path.toStdString());

    QTextStream in(&file);
    QString script = in.readAll();

    QScriptValue result = _scriptEngine->evaluate(script);

    if (_scriptEngine->hasUncaughtException()) {
        QString error = _scriptEngine->uncaughtException().toString();
        int a = 4545;
    }
}

/**
 * @brief Events dispatcher
 */
bool MongoClient::event(QEvent *event)
{
    R_HANDLE(event)
    R_EVENT(EstablishConnectionRequest)
    R_EVENT(LoadDatabaseNamesRequest)
    R_EVENT(LoadCollectionNamesRequest)
    R_EVENT(ExecuteQueryRequest)
    R_EVENT(ExecuteScriptRequest)
    R_EVENT(InitRequest)
    else return QObject::event(event);
}

void MongoClient::handle(InitRequest *event)
{
    try {
        _scriptEngine = new QScriptEngine();

        _helper = new Helper();
        QScriptValue helper = _scriptEngine->newQObject(_helper);
        _scriptEngine->globalObject().setProperty("helper", helper);

        evaluteFile(":/robomongo/scripts/robo.js");
        //evaluteFile(":/robomongo/scripts/utils.js");
        evaluteFile(":/robomongo/scripts/db.js");
        evaluteFile(":/robomongo/scripts/mongo.js");
        evaluteFile(":/robomongo/scripts/collection.js");
        evaluteFile(":/robomongo/scripts/query.js");
        evaluteFile(":/robomongo/scripts/mr.js");


    }
    catch (std::exception &ex) {
        reply(event->sender, new InitResponse(Error("Unable to initialize MongoClient")));
    }
}

/**
 * @brief Initiate connection to MongoDB
 */
void MongoClient::handle(EstablishConnectionRequest *event)
{
    QMutexLocker lock(&_firstConnectionMutex);

    try
    {
        boost::scoped_ptr<ScopedDbConnection> conn(ScopedDbConnection::getScopedDbConnection(_address.toStdString()));
        conn->done();

        reply(event->sender, new EstablishConnectionResponse(_address));
    }
    catch(DBException &ex)
    {
        reply(event->sender, new EstablishConnectionResponse(Error("Unable to connect to MongoDB")));
    }
}

/**
 * @brief Load list of all database names
 */
void MongoClient::handle(LoadDatabaseNamesRequest *event)
{
    try
    {
        boost::scoped_ptr<ScopedDbConnection> conn(ScopedDbConnection::getScopedDbConnection(_address.toStdString()));
        list<string> dbs = conn->get()->getDatabaseNames();
        conn->done();

        QStringList stringList;
        for ( list<string>::iterator i = dbs.begin(); i != dbs.end(); i++ ) {
            stringList.append(QString::fromStdString(*i));
        }

        reply(event->sender, new LoadDatabaseNamesResponse(stringList));
    }
    catch(DBException &ex)
    {
        reply(event->sender, new LoadDatabaseNamesResponse(Error("Unable to load database names.")));
    }
}

/**
 * @brief Load list of all collection names
 */
void MongoClient::handle(LoadCollectionNamesRequest *event)
{
    try
    {
        boost::scoped_ptr<ScopedDbConnection> conn(ScopedDbConnection::getScopedDbConnection(_address.toStdString()));
        list<string> dbs = conn->get()->getCollectionNames(event->databaseName.toStdString());
        conn->done();

        QStringList stringList;
        for ( list<string>::iterator i = dbs.begin(); i != dbs.end(); i++ ) {
            stringList.append(QString::fromStdString(*i));
        }

        reply(event->sender, new LoadCollectionNamesResponse(event->databaseName, stringList));
    }
    catch(DBException &ex)
    {
        reply(event->sender, new LoadCollectionNamesResponse(Error("Unable to load list of collections.")));
    }
}

void MongoClient::handle(ExecuteQueryRequest *event)
{
    try
    {
        boost::scoped_ptr<ScopedDbConnection> conn(ScopedDbConnection::getScopedDbConnection(_address.toStdString()));

        QList<BSONObj> docs;
        auto_ptr<DBClientCursor> cursor = conn->get()->query(event->nspace.toStdString(), BSONObj(), 100);
        while (cursor->more())
        {
            BSONObj bsonObj = cursor->next();
            docs.append(bsonObj);
        }

        conn->done();

        reply(event->sender, new ExecuteQueryResponse(docs));
    }
    catch(DBException &ex)
    {
        reply(event->sender, new ExecuteQueryResponse(Error("Unable to complete query.")));
    }
}

/**
 * @brief Execute javascript
 */
void MongoClient::handle(ExecuteScriptRequest *event)
{
    try
    {
        QScriptValue value = _scriptEngine->evaluate(event->script);

        if (!value.isError())
            reply(event->sender, new ExecuteScriptResponse(_helper->text()));

//        boost::scoped_ptr<ScopedDbConnection> conn(ScopedDbConnection::getScopedDbConnection(_address.toStdString()));

//        QList<BSONObj> docs;
//        auto_ptr<DBClientCursor> cursor = conn->get()->query(event->nspace.toStdString(), BSONObj(), 100);
//        while (cursor->more())
//        {
//            BSONObj bsonObj = cursor->next();
//            docs.append(bsonObj);
//        }

//        conn->done();

//        reply(event->sender, new ExecuteScriptResponse(docs));
    }
    catch(DBException &ex)
    {
        reply(event->sender, new ExecuteScriptResponse(Error("Unable to complete query.")));
    }
}

/**
 * @brief Send event to this MongoClient
 */
void MongoClient::send(QEvent *event)
{
    QCoreApplication::postEvent(this, event);
}

/**
 * @brief Send reply event to object 'receiver'
 */
void MongoClient::reply(QObject *receiver, QEvent *event)
{
    QCoreApplication::postEvent(receiver, event);
}
