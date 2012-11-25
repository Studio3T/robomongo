#include "MongoClient.h"
#include <QThread>
#include <QStringList>
#include "boost/scoped_ptr.hpp"
#include "mongo/client/dbclient.h"
#include <QMutexLocker>
#include <QCoreApplication>
#include "events/MongoEvents.h"

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

        reply(event->sender(), new EstablishConnectionResponse(_address));
    }
    catch(DBException &ex)
    {
        reply(event->sender(), new EstablishConnectionResponse(Error("Unable to connect to MongoDB")));
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

        reply(event->sender(), new LoadDatabaseNamesResponse(stringList));
    }
    catch(DBException &ex)
    {
        reply(event->sender(), new LoadDatabaseNamesResponse(Error("Unable to load database names.")));
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

        reply(event->sender(), new LoadCollectionNamesResponse(event->databaseName, stringList));
    }
    catch(DBException &ex)
    {
        reply(event->sender(), new LoadCollectionNamesResponse(Error("Unable to load list of collections.")));
    }
}

/**
 * @brief Events dispatcher
 */
bool MongoClient::event(QEvent *event)
{
    R_HANDLE(event) {
        R_EVENT(EstablishConnectionRequest);
        R_EVENT(LoadDatabaseNamesRequest);
        R_EVENT(LoadCollectionNamesRequest);
    }

    QObject::event(event);
}

/**
 * @brief Send event to this MongoClient
 */
void MongoClient::send(QEvent *event)
{
    QCoreApplication::postEvent(this, event);
}

/**
 * @brief Send reply event to object 'obj'
 */
void MongoClient::reply(QObject *obj, QEvent *event)
{
    QCoreApplication::postEvent(obj, event);
}
