#include "robomongo/core/mongodb/MongoWorker.h"

#include <QDebug>
#include <QThread>
#include <QStringList>
#include <QMutexLocker>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QVector>
#include "boost/scoped_ptr.hpp"
#include <mongo/client/dbclient.h>
#include <mongo/scripting/engine_spidermonkey.h>

#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/engine/ScriptEngine.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/mongodb/MongoWorkerThread.h"
#include "robomongo/core/mongodb/MongoClient.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/domain/MongoCollectionInfo.h"

using namespace Robomongo;
using namespace std;
using namespace mongo;


MongoWorker::MongoWorker(EventBus *bus, ConnectionSettings *connection, QObject *parent) : QObject(parent),
    _connection(connection),
    _bus(bus),
    _scriptEngine(NULL)
{
    _address = _connection->getFullAddress();
    init();
}

MongoWorker::~MongoWorker()
{
    delete _connection;
    _thread->quit();
    if (!_thread->wait(1000))
        _thread->terminate();

    delete _thread;
}

/**
 * @brief Initialise MongoWorker
 */
void MongoWorker::init()
{
    qDebug() << "MongoWorker init started";
    _isAdmin = true;
    _thread = new MongoWorkerThread(this);
    this->moveToThread(_thread);

    _thread->start();
    qDebug() << "MongoWorker init finished";
}

void MongoWorker::handle(InitRequest *event)
{
    try {
        qDebug() << "InitRequest received";
        _scriptEngine = new ScriptEngine(_connection);
        _scriptEngine->init();
        _scriptEngine->use(_connection->defaultDatabase());
    }
    catch (std::exception &ex) {
        qDebug() << "InitRequest handler throw exception: " << ex.what();
        reply(event->sender(), new InitResponse(this, EventError("Unable to initialize MongoWorker")));
    }
}

void MongoWorker::handle(FinalizeRequest *event)
{
    try {

    }
    catch (std::exception &ex) {

    }
}

/**
 * @brief Initiate connection to MongoDB
 */
void MongoWorker::handle(EstablishConnectionRequest *event)
{
    qDebug() << "EstablishConnectionRequest received";
    QMutexLocker lock(&_firstConnectionMutex);

    try {
        qDebug() << "EstablishConnectionRequest in try block";
        boost::scoped_ptr<ScopedDbConnection> conn(getConnection());

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
        qDebug() << "EstablishConnectionResponse sent back";
    } catch(std::exception &ex) {
        qDebug() << "EstablishConnectionRequest throw exception: " << ex.what();
        reply(event->sender(), new EstablishConnectionResponse(this, EventError("Unable to connect to MongoDB")));
    }
}

/**
 * @brief Load list of all database names
 */
void MongoWorker::handle(LoadDatabaseNamesRequest *event)
{
    try {
        // If user not an admin - he doesn't have access to mongodb 'listDatabases' command
        // Non admin user has access only to the single database he specified while performing auth.
        if (!_isAdmin) {
            QStringList dbNames;
            dbNames << _authDatabase;
            reply(event->sender(), new LoadDatabaseNamesResponse(this, dbNames));
            return;
        }

        boost::scoped_ptr<MongoClient> client(getClient());
        QStringList dbNames = client->getDatabaseNames();
        client->done();

        reply(event->sender(), new LoadDatabaseNamesResponse(this, dbNames));
    } catch(DBException &ex) {
        reply(event->sender(), new LoadDatabaseNamesResponse(this, EventError("Unable to load database names.")));
    }
}

/**
 * @brief Load list of all collection names
 */
void MongoWorker::handle(LoadCollectionNamesRequest *event)
{
    try {
        boost::scoped_ptr<MongoClient> client(getClient());

        QStringList stringList = client->getCollectionNames(event->databaseName());
        QList<MongoCollectionInfo> infos = client->runCollStatsCommand(stringList);
        client->done();

        reply(event->sender(), new LoadCollectionNamesResponse(this, event->databaseName(), infos));
    } catch(DBException &ex) {
        reply(event->sender(), new LoadCollectionNamesResponse(this, EventError("Unable to load list of collections.")));
    }
}

void MongoWorker::handle(ExecuteQueryRequest *event)
{
    try {
        boost::scoped_ptr<MongoClient> client(getClient());
        QList<MongoDocumentPtr> docs = client->query(event->queryInfo());
        client->done();

        reply(event->sender(), new ExecuteQueryResponse(this, event->resultIndex(), event->queryInfo(), docs));
    } catch(DBException &ex) {
        reply(event->sender(), new ExecuteQueryResponse(this, EventError("Unable to complete query.")));
    }
}

/**
 * @brief Execute javascript
 */
void MongoWorker::handle(ExecuteScriptRequest *event)
{
    try {
        MongoShellExecResult result = _scriptEngine->exec(event->script, event->databaseName);
        reply(event->sender(), new ExecuteScriptResponse(this, result, event->script.isEmpty()));
    } catch(DBException &ex) {
        reply(event->sender(), new ExecuteScriptResponse(this, EventError("Unable to complete query.")));
    }
}

void MongoWorker::handle(AutocompleteRequest *event)
{
    try {
        QStringList list = _scriptEngine->complete(event->prefix);
        reply(event->sender(), new AutocompleteResponse(this, list));
    } catch(DBException &ex) {
        reply(event->sender(), new ExecuteScriptResponse(this, EventError("Unable to autocomplete query.")));
    }
}

ScopedDbConnection *MongoWorker::getConnection()
{
    return ScopedDbConnection::getScopedDbConnection(_address.toStdString());
}

MongoClient *MongoWorker::getClient()
{
    return new MongoClient(getConnection());
}

/**
 * @brief Send event to this MongoWorker
 */
void MongoWorker::send(Event *event)
{
    _bus->send(this, event);
}

/**
 * @brief Send reply event to object 'receiver'
 */
void MongoWorker::reply(QObject *receiver, Event *event)
{
    _bus->send(receiver, event);
}
