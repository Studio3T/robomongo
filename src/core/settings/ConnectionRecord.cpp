#include "ConnectionRecord.h"

using namespace Robomongo;

/**
 * Creates ConnectionRecord with default values
 */
ConnectionRecord::ConnectionRecord() : _data(new ConnectionRecordPrivate)
{
    _data->id = 0;
    _data->databasePort = 27017;
}

ConnectionRecord * ConnectionRecord::clone()
{
	ConnectionRecord * connection = new ConnectionRecord;
	connection->setConnectionName(connectionName());
	connection->setDatabaseAddress(databaseAddress());
	connection->setDatabasePort(databasePort());
	connection->setUserName(userName());
	connection->setUserPassword(userPassword());
    return connection;
}

QVariant ConnectionRecord::toVariant() const
{
    QVariantMap map;
    map.insert("connectionName", connectionName());
    map.insert("databaseAddress", databaseAddress());
    map.insert("databasePort", databasePort());
    map.insert("userName", userName());
    map.insert("userPassword", userPassword());
    return map;
}

void ConnectionRecord::fromVariant(QVariantMap map)
{
    setConnectionName(map.value("connectionName").toString());
    setDatabaseAddress(map.value("databaseAddress").toString());
    setDatabasePort(map.value("databasePort").toInt());
    setUserName(map.value("userName").toString());
    setUserPassword(map.value("userPassword").toString());
}
