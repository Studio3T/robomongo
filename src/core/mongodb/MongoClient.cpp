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
#include "engine/ScriptEngine.h"
#include "mongo/scripting/engine_spidermonkey.h"
#include <QVector>
#include "EventBus.h"
#include "MongoClientThread.h"
#include "settings/ConnectionSettings.h"
#include "domain/MongoShellResult.h"

using namespace Robomongo;
using namespace std;
using namespace mongo;


MongoClient::MongoClient(EventBus *bus, ConnectionSettings *connection, QObject *parent) : QObject(parent),
    _connection(connection),
    _bus(bus),
    _scriptEngine(NULL)
{
    _address = _connection->getFullAddress();
    init();
}

MongoClient::~MongoClient()
{
    delete _connection;
    _thread->quit();
    if (!_thread->wait(1000))
        _thread->terminate();

    delete _thread;
}

/**
 * @brief Initialise MongoClient
 */
void MongoClient::init()
{
    _isAdmin = true;
    _thread = new MongoClientThread(this);
    this->moveToThread(_thread);

    _thread->start();
}

void MongoClient::evaluteFile(const QString &path)
{
    /*
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Unable to read " + path.toStdString());

    QTextStream in(&file);
    QString script = in.readAll();

    QScriptValue result = _scriptEngine->evaluate(script);

    if (_scriptEngine->hasUncaughtException()) {
        QString error = _scriptEngine->uncaughtException().toString();
        int a = 4545;
    }*/
}

void MongoClient::handle(InitRequest *event)
{
    try {
        _scriptEngine = new ScriptEngine(_connection);
        _scriptEngine->init();
        _scriptEngine->use(_connection->defaultDatabase());
    }
    catch (std::exception &ex) {
        reply(event->sender(), new InitResponse(this, EventError("Unable to initialize MongoClient")));
    }
}

void MongoClient::handle(FinalizeRequest *event)
{
    try {

    }
    catch (std::exception &ex) {

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

        if (_connection->hasEnabledPrimaryCredential())
        {
            std::string errmsg;
            bool ok = conn->get()->auth(
                _connection->primaryCredential()->databaseName().toStdString(),
                _connection->primaryCredential()->userName().toStdString(),
                _connection->primaryCredential()->userPassword().toStdString(), errmsg);

            if (!ok)
            {
                QString lastErrorMessage = QString::fromStdString(errmsg);
                throw runtime_error("Unable to authorize");
            }

            // If authentication succeed and database name is 'admin' -
            // then user is admin, otherwise user is not admin
            if (_connection->primaryCredential()->databaseName().compare("admin", Qt::CaseInsensitive))
                _isAdmin = false;

            // Save name of db on which we authenticated
            _authDatabase = _connection->primaryCredential()->databaseName();
        }

        conn->done();
        reply(event->sender(), new EstablishConnectionResponse(this, _address));
    }
    catch(std::exception &ex)
    {
        reply(event->sender(), new EstablishConnectionResponse(this, EventError("Unable to connect to MongoDB")));
    }
}

/**
 * @brief Load list of all database names
 */
void MongoClient::handle(LoadDatabaseNamesRequest *event)
{
    try
    {
        // If user not an admin - he doesn't have access to mongodb 'listDatabases' command
        // Non admin user has access only to the single database he specified while performing auth.
        if (!_isAdmin) {
            QStringList dbNames;
            dbNames << _authDatabase;
            reply(event->sender(), new LoadDatabaseNamesResponse(this, dbNames));
            return;
        }

        boost::scoped_ptr<ScopedDbConnection> conn(ScopedDbConnection::getScopedDbConnection(_address.toStdString()));
        list<string> dbs = conn->get()->getDatabaseNames();
        conn->done();

        QStringList dbNames;
        for ( list<string>::iterator i = dbs.begin(); i != dbs.end(); i++ ) {
            dbNames.append(QString::fromStdString(*i));
        }

        dbNames.sort();

        reply(event->sender(), new LoadDatabaseNamesResponse(this, dbNames));
    }
    catch(DBException &ex)
    {
        reply(event->sender(), new LoadDatabaseNamesResponse(this, EventError("Unable to load database names.")));
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

        stringList.sort();
        reply(event->sender(), new LoadCollectionNamesResponse(this, event->databaseName, stringList));
    }
    catch(DBException &ex)
    {
        reply(event->sender(), new LoadCollectionNamesResponse(this, EventError("Unable to load list of collections.")));
    }
}

void MongoClient::handle(ExecuteQueryRequest *event)
{
    try
    {
        boost::scoped_ptr<ScopedDbConnection> conn(ScopedDbConnection::getScopedDbConnection(_address.toStdString()));

        QueryInfo &info = event->queryInfo;

        QString ns = QString("%1.%2").arg(info.databaseName, info.collectionName);

        QList<BSONObj> docs;
        auto_ptr<DBClientCursor> cursor = conn->get()->query(
            ns.toStdString(), info.query, info.limit, info.skip,
            info.fields.nFields() ? &info.fields : 0, info.options, info.batchSize);

        while (cursor->more())
        {
            BSONObj &bsonObj = cursor->next();
            docs.append(bsonObj.getOwned());
        }

        conn->done();

        reply(event->sender(), new ExecuteQueryResponse(this, event->resultIndex, info, docs));
    }
    catch(DBException &ex)
    {
        reply(event->sender(), new ExecuteQueryResponse(this, EventError("Unable to complete query.")));
    }
}

/**
 * @brief Execute javascript
 */
void MongoClient::handle(ExecuteScriptRequest *event)
{
    try
    {
        ExecResult result = _scriptEngine->exec(event->script, event->databaseName);
        reply(event->sender(), new ExecuteScriptResponse(this, result, event->script.isEmpty()));
    }
    catch(DBException &ex)
    {
        reply(event->sender(), new ExecuteScriptResponse(this, EventError("Unable to complete query.")));
    }
}

/**
 * @brief Send event to this MongoClient
 */
void MongoClient::send(Event *event)
{
    _bus->send(this, event);
    //const char * typeName = event->typeString();
    //QMetaObject::invokeMethod(this, "handle", Qt::QueuedConnection, QGenericArgument(typeName, &event));

    // was:
    // QCoreApplication::postEvent(this, event);
}

/**
 * @brief Send reply event to object 'receiver'
 */
void MongoClient::reply(QObject *receiver, Event *event)
{
    _bus->send(receiver, event);
    //const char * typeName = event->typeString();
    //QMetaObject::invokeMethod(receiver, "handle", Qt::QueuedConnection, QGenericArgument(typeName, &event));

    // was:
    // QCoreApplication::postEvent(receiver, event);
}
