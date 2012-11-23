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

void MongoClient::init()
{
    _thread = new QThread();
    this->moveToThread(_thread);

    _thread->start();
}


/**
 * @brief Actual implementation of loadDatabaseNames()
 */
void MongoClient::_loadDatabaseNames(QObject *sender)
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

        reply(sender, new DatabaseNamesLoaded(stringList));
    }
    catch(DBException &ex)
    {
        reply(sender, new ConnectionFailed(_address));
    }
}

void MongoClient::_loadCollectionNames(QObject *sender, const QString &databaseName)
{
    try
    {
        boost::scoped_ptr<ScopedDbConnection> conn(ScopedDbConnection::getScopedDbConnection(_address.toStdString()));
        list<string> dbs = conn->get()->getCollectionNames(databaseName.toStdString());
        conn->done();

        QStringList stringList;
        for ( list<string>::iterator i = dbs.begin(); i != dbs.end(); i++ ) {
            stringList.append(QString::fromStdString(*i));
        }

        reply(sender, new CollectionNamesLoaded(databaseName, stringList));
    }
    catch(DBException &ex)
    {
        reply(sender, new ConnectionFailed(_address));
    }
}


void MongoClient::_establishConnection(QObject *sender)
{
    QMutexLocker lock(&_firstConnectionMutex);

    try
    {
        boost::scoped_ptr<ScopedDbConnection> conn(ScopedDbConnection::getScopedDbConnection(_address.toStdString()));
        conn->done();

        reply(sender, new ConnectionEstablished(_address));
    }
    catch(DBException &ex)
    {
        reply(sender, new ConnectionFailed(_address));
    }
}



void MongoClient::establishConnection(QObject *sender)
{
    invoke("_establishConnection", Q_ARG(QObject *, sender));
}

/**
 * @brief Load list of all database names
 */
void MongoClient::loadDatabaseNames(QObject *sender)
{
    invoke("_loadDatabaseNames", Q_ARG(QObject *, sender));
}

void MongoClient::loadCollectionNames(QObject *sender, const QString &databaseName)
{
    invoke("_loadCollectionNames", Q_ARG(QObject *, sender), Q_ARG(QString, databaseName));
}

void MongoClient::reply(QObject *obj, QEvent *event)
{
    QCoreApplication::postEvent(obj, event);
}

void MongoClient::invoke(char *methodName, QGenericArgument arg1, QGenericArgument arg2, QGenericArgument arg3,
                                           QGenericArgument arg4,QGenericArgument arg5)
{
    QMetaObject::invokeMethod(this, methodName, Qt::QueuedConnection, arg1, arg2, arg3, arg4, arg5);
}
