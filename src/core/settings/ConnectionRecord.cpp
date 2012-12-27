#include "ConnectionRecord.h"

using namespace Robomongo;

/**
 * @brief Creates ConnectionRecord with default values
 */
ConnectionRecord::ConnectionRecord() : QObject()
{
    _id = 0;
    _databasePort = 27017;
}

/**
 * @brief Creates completely new ConnectionRecord by cloning this record.
 */
ConnectionRecord *ConnectionRecord::clone() const
{
    ConnectionRecord *record = new ConnectionRecord();
    record->setConnectionName(connectionName());
    record->setDatabaseAddress(databaseAddress());
    record->setDatabasePort(databasePort());
    record->setUserName(userName());
    record->setUserPassword(userPassword());
    record->setDatabaseName(databaseName());
    return record;
}

/**
 * @brief Converts to QVariantMap
 */
QVariant ConnectionRecord::toVariant() const
{
    QVariantMap map;
    map.insert("connectionName", connectionName());
    map.insert("databaseAddress", databaseAddress());
    map.insert("databasePort", databasePort());
    map.insert("userName", userName());
    map.insert("userPassword", userPassword());
    map.insert("databaseName", databaseName());
    return map;
}

/**
 * @brief Converts from QVariantMap (and clean current state)
 */
void ConnectionRecord::fromVariant(QVariantMap map)
{
    setConnectionName(map.value("connectionName").toString());
    setDatabaseAddress(map.value("databaseAddress").toString());
    setDatabasePort(map.value("databasePort").toInt());
    setUserName(map.value("userName").toString());
    setUserPassword(map.value("userPassword").toString());
    setDatabaseName(map.value("databaseName").toString());
}

