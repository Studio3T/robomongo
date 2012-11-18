#include <QStringList>
#include "MongoServer.h"
#include "MongoException.h"

using namespace Robomongo;
using namespace std;

MongoServer::MongoServer(const QString &host, const QString &port) : QObject()
{
    _host = host;
    _port = port;
    _address = QString("%1:%2").arg(host).arg(port);

    _connection.reset(new mongo::DBClientConnection);
}

/**
 * @brief Try to connect to MongoDB server.
 * @throws MongoException, if fails
 */
void MongoServer::tryConnect()
{
    try {
        _connection->connect(_address.toStdString());
    }
    catch (mongo::UserException & e) {
        throw MongoException("Unable to connect");
    }
}

/**
 * @brief Try to connect to MongoDB server.
 * @throws MongoException, if fails
 */
bool MongoServer::authenticate(const QString &database, const QString &username, const QString &password)
{
    std::string errmsg;
    bool ok = _connection->auth(database.toStdString(), username.toStdString(), password.toStdString(), errmsg);

    if (!ok)
    {
        _lastErrorMessage = QString::fromStdString(errmsg);
        throw MongoException("Unable to authorize");
    }
}

/**
 * @brief Load list of all database names
 */
QStringList MongoServer::databaseNames()
{
    QStringList stringList;

    list<string> dbs = _connection->getDatabaseNames();

    for ( list<string>::iterator i = dbs.begin(); i != dbs.end(); i++ ) {
        stringList.append(QString::fromStdString(*i));
    }

    return stringList;
}
