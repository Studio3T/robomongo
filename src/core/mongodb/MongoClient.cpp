#include "MongoClient.h"
#include <QThread>
#include <QStringList>
#include "boost/scoped_ptr.hpp"
#include "mongo/client/dbclient.h"

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

void MongoClient::init()
{
    _thread = new QThread();
    this->moveToThread(_thread);

    _thread->start();
}

void MongoClient::invoke(char *methodName, QGenericArgument arg1, QGenericArgument arg2, QGenericArgument arg3,
                                           QGenericArgument arg4,QGenericArgument arg5)
{
    QMetaObject::invokeMethod(this, methodName, Qt::QueuedConnection, arg1, arg2, arg3, arg4, arg5);
}

/**
 * @brief Load list of all database names
 */
void MongoClient::loadDatabaseNames()
{
    invoke("_loadDatabaseNames");
}

/**
 * @brief Actual implementation of loadDatabaseNames()
 */
void MongoClient::_loadDatabaseNames()
{
    boost::scoped_ptr<ScopedDbConnection> conn(ScopedDbConnection::getScopedDbConnection(_address.toStdString()));

    QStringList stringList;

    list<string> dbs = conn->get()->getDatabaseNames();

    conn->done();

    for ( list<string>::iterator i = dbs.begin(); i != dbs.end(); i++ ) {
        stringList.append(QString::fromStdString(*i));
    }

    emit databaseNamesLoaded(stringList);
}
