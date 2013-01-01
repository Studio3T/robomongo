#include "ConnectionSettings.h"

using namespace Robomongo;

/**
 * @brief Creates ConnectionSettings with default values
 */
ConnectionSettings::ConnectionSettings() : QObject()
{
    _databasePort = 27017;
}

/**
 * @brief Creates completely new ConnectionSettings by cloning this record.
 */
ConnectionSettings *ConnectionSettings::clone() const
{
    ConnectionSettings *record = new ConnectionSettings();
    record->setConnectionName(connectionName());
    record->setDatabaseAddress(databaseAddress());
    record->setDatabasePort(databasePort());

    foreach(CredentialSettings *credential, credentials()) {
        record->addCredential(credential->clone());
    }

    return record;
}

/**
 * @brief Converts to QVariantMap
 */
QVariant ConnectionSettings::toVariant() const
{
    QVariantMap map;
    map.insert("connectionName", connectionName());
    map.insert("databaseAddress", databaseAddress());
    map.insert("databasePort", databasePort());

    QVariantList list;
    foreach(CredentialSettings *credential, credentials()) {
        list.append(credential->toVariant());
    }
    map.insert("credentials", list);

    return map;
}

/**
 * @brief Converts from QVariantMap (and clean current state)
 */
void ConnectionSettings::fromVariant(QVariantMap map)
{
    setConnectionName(map.value("connectionName").toString());
    setDatabaseAddress(map.value("databaseAddress").toString());
    setDatabasePort(map.value("databasePort").toInt());

    QVariantList list = map.value("credentials").toList();
    foreach(QVariant var, list) {
        CredentialSettings *credential = new CredentialSettings(var.toMap());
        addCredential(credential);
    }
}

/**
 * @brief Adds credential to this connection
 */
void ConnectionSettings::addCredential(CredentialSettings *credential)
{
    if (_credentialsByDatabaseName.contains(credential->databaseName()))
        return;

    _credentials.append(credential);
    _credentialsByDatabaseName.insert(credential->databaseName(), credential);
}

/**
 * @brief Returns credential for specified database, or NULL if no such
 * credential in connection.
 */
CredentialSettings *ConnectionSettings::credential(QString databaseName)
{
    return _credentialsByDatabaseName.value(databaseName);
}

CredentialSettings *ConnectionSettings::credential(int index)
{
    if (index >= _credentials.count())
        return NULL;

    return _credentials.value(index);
}

bool ConnectionSettings::hasEnabledCredential()
{
    if (_credentials.count() == 0)
        return false;

    return firstCredential()->enabled();
}

CredentialSettings *ConnectionSettings::firstCredential()
{
    if (_credentials.count() == 0)
        return NULL;

    return _credentials.at(0);
}

