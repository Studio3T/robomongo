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
    _scriptEngine(NULL),
    _dbclient(NULL)
{
    _address = _connection->getFullAddress();
    init();
}

MongoWorker::~MongoWorker()
{
    if (_dbclient)
        delete _dbclient;

    delete _connection;
    _thread->quit();
    if (!_thread->wait(2000))
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

        _keepAliveTimer = new QTimer(this);
        connect(_keepAliveTimer, SIGNAL(timeout()), this, SLOT(keepAlive()));
        _keepAliveTimer->start(60 * 1000); // every minute
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
        mongo::DBClientBase *conn = getConnection();

        if (_connection->hasEnabledPrimaryCredential())
        {
            std::string errmsg;
            bool ok = conn->auth(
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

        //conn->done();
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

void MongoWorker::handle(LoadUsersRequest *event)
{
    try {
        boost::scoped_ptr<MongoClient> client(getClient());
        QList<MongoUser> users = client->getUsers(event->databaseName());
        client->done();

        reply(event->sender(), new LoadUsersResponse(this, event->databaseName(), users));
    } catch(DBException &ex) {
        reply(event->sender(), new LoadUsersResponse(this, EventError("Unable to load list of users.")));
    }
}

void MongoWorker::handle(LoadFunctionsRequest *event)
{
    try {
        boost::scoped_ptr<MongoClient> client(getClient());
        QList<MongoFunction> funs = client->getFunctions(event->databaseName());
        client->done();

        reply(event->sender(), new LoadFunctionsResponse(this, event->databaseName(), funs));
    } catch(DBException &ex) {
        reply(event->sender(), new LoadFunctionsResponse(this, EventError("Unable to load list of functions.")));
    }
}

void MongoWorker::handle(InsertDocumentRequest *event)
{
    try {
        boost::scoped_ptr<MongoClient> client(getClient());

        if (event->overwrite())
            client->saveDocument(event->obj(), event->database(), event->collection());
        else
            client->insertDocument(event->obj(), event->database(), event->collection());

        client->done();

        reply(event->sender(), new InsertDocumentResponse(this));
    } catch(DBException &ex) {
        reply(event->sender(), new InsertDocumentResponse(this, EventError("Unable to insert document.")));
    }
}

void MongoWorker::handle(RemoveDocumentRequest *event)
{
    try {
        boost::scoped_ptr<MongoClient> client(getClient());

        client->removeDocuments(event->database(), event->collection(), event->query(), event->justOne());
        client->done();

        reply(event->sender(), new RemoveDocumentResponse(this));
    } catch(DBException &ex) {
        reply(event->sender(), new RemoveDocumentResponse(this, EventError("Unable to remove documents.")));
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
        reply(event->sender(), new AutocompleteResponse(this, list, event->prefix));
    } catch(DBException &ex) {
        reply(event->sender(), new ExecuteScriptResponse(this, EventError("Unable to autocomplete query.")));
    }
}

void MongoWorker::handle(CreateDatabaseRequest *event)
{
    try {
        boost::scoped_ptr<MongoClient> client(getClient());
        client->createDatabase(event->database());
        client->done();

        reply(event->sender(), new CreateDatabaseResponse(this));
    } catch(DBException &ex) {
        reply(event->sender(), new CreateDatabaseResponse(this, EventError("Unable to create database.")));
    }
}

void MongoWorker::handle(DropDatabaseRequest *event)
{
    try {
        boost::scoped_ptr<MongoClient> client(getClient());
        client->dropDatabase(event->database());
        client->done();

        reply(event->sender(), new DropDatabaseResponse(this));
    } catch(DBException &ex) {
        reply(event->sender(), new DropDatabaseResponse(this, EventError("Unable to drop database.")));
    }
}

void MongoWorker::handle(CreateCollectionRequest *event)
{
    try {
        boost::scoped_ptr<MongoClient> client(getClient());
        client->createCollection(event->database(), event->collection());
        client->done();

        reply(event->sender(), new CreateCollectionResponse(this));
    } catch(DBException &ex) {
        reply(event->sender(), new CreateCollectionResponse(this, EventError("Unable to create collection.")));
    }
}

void MongoWorker::handle(DropCollectionRequest *event)
{
    try {
        boost::scoped_ptr<MongoClient> client(getClient());
        client->dropCollection(event->database(), event->collection());
        client->done();

        reply(event->sender(), new DropCollectionResponse(this));
    } catch(DBException &ex) {
        reply(event->sender(), new DropCollectionResponse(this, EventError("Unable to drop collection.")));
    }
}

void MongoWorker::handle(RenameCollectionRequest *event)
{
    try {
        boost::scoped_ptr<MongoClient> client(getClient());
        client->renameCollection(event->database(), event->collection(), event->newCollection());
        client->done();

        reply(event->sender(), new RenameCollectionResponse(this));
    } catch(DBException &ex) {
        reply(event->sender(), new RenameCollectionResponse(this, EventError("Unable to rename collection.")));
    }
}

void MongoWorker::handle(DuplicateCollectionRequest *event)
{
    try {
        boost::scoped_ptr<MongoClient> client(getClient());
        client->duplicateCollection(event->database(), event->collection(), event->newCollection());
        client->done();

        reply(event->sender(), new DuplicateCollectionResponse(this));
    } catch(DBException &ex) {
        reply(event->sender(), new DuplicateCollectionResponse(this, EventError("Unable to duplicate collection.")));
    }
}

void MongoWorker::handle(CreateUserRequest *event)
{
    try {
        boost::scoped_ptr<MongoClient> client(getClient());
        client->createUser(event->database(), event->user(), event->overwrite());
        client->done();

        reply(event->sender(), new CreateUserResponse(this));
    } catch(DBException &ex) {
        reply(event->sender(), new CreateUserResponse(this, EventError("Unable to create/ovewrite user.")));
    }
}

void MongoWorker::handle(DropUserRequest *event)
{
    try {
        boost::scoped_ptr<MongoClient> client(getClient());
        client->dropUser(event->database(), event->id());
        client->done();

        reply(event->sender(), new DropUserResponse(this));
    } catch(DBException &ex) {
        reply(event->sender(), new DropUserResponse(this, EventError("Unable to drop user.")));
    }
}

void MongoWorker::handle(CreateFunctionRequest *event)
{
    try {
        boost::scoped_ptr<MongoClient> client(getClient());
        client->createFunction(event->database(), event->function(), event->existingFunctionName());
        client->done();

        reply(event->sender(), new CreateFunctionResponse(this));
    } catch(DBException &ex) {
        reply(event->sender(), new CreateFunctionResponse(this, EventError("Unable to create/ovewrite function.")));
    }
}

void MongoWorker::handle(DropFunctionRequest *event)
{
    try {
        boost::scoped_ptr<MongoClient> client(getClient());
        client->dropFunction(event->database(), event->name());
        client->done();

        reply(event->sender(), new DropFunctionResponse(this));
    } catch(DBException &ex) {
        reply(event->sender(), new DropFunctionResponse(this, EventError("Unable to drop function.")));
    }
}

mongo::DBClientBase *MongoWorker::getConnection()
{
    if (!_dbclient) {
        mongo::DBClientConnection *conn = new mongo::DBClientConnection(true);
        conn->connect(_address.toStdString());
        _dbclient = conn;
    }
    return _dbclient;
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

void MongoWorker::keepAlive()
{
    try {
        if (_dbclient) {
            // Building { ping: 1 }
            mongo::BSONObjBuilder command;
            command.append("ping", 1);
            mongo::BSONObj result;

            if (_authDatabase.isEmpty()) {
                _dbclient->runCommand("admin", command.obj(), result);
            } else {
                _dbclient->runCommand(_authDatabase.toStdString(), command.obj(), result);
            }
        }

        if (_scriptEngine) {
            _scriptEngine->ping();
        }

    } catch(std::exception &ex) {
        // nothing here
    }
}

/**
 * @brief Send reply event to object 'receiver'
 */
void MongoWorker::reply(QObject *receiver, Event *event)
{
    _bus->send(receiver, event);
}
