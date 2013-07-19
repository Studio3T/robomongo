#include "robomongo/core/settings/ConnectionSettings.h"

namespace
{
    const unsigned port = 27017;
}

namespace Robomongo
{

    /**
     * @brief Creates ConnectionSettings with default values
     */
    ConnectionSettings::ConnectionSettings() : QObject(),
        _serverPort(port),
        _serverHost("localhost"),
        _defaultDatabase(""),
        _connectionName("New Connection") { }

    /**
     * @brief Cleanup used resources
     */
    ConnectionSettings::~ConnectionSettings()
    {
        clearCredentials();
    }

    /**
     * @brief Creates completely new ConnectionSettings by cloning this record.
     */
    ConnectionSettings *ConnectionSettings::clone() const
    {
        ConnectionSettings *record = new ConnectionSettings();
        record->apply(this);
        return record;
    }

    /**
     * @brief Discards current state and applies state from 'source' ConnectionSettings.
     */
    void ConnectionSettings::apply(const ConnectionSettings *source)
    {
        setConnectionName(source->connectionName());
        setServerHost(source->serverHost());
        setServerPort(source->serverPort());
        setDefaultDatabase(source->defaultDatabase());

        clearCredentials();
        foreach(CredentialSettings *credential, source->credentials()) {
            addCredential(credential->clone());
        }
    }

    /**
     * @brief Converts to QVariantMap
     */
    QVariant ConnectionSettings::toVariant() const
    {
        QVariantMap map;
        map.insert("connectionName", connectionName());
        map.insert("serverHost", serverHost());
        map.insert("serverPort", serverPort());
        map.insert("defaultDatabase", defaultDatabase());

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
        setServerHost(map.value("serverHost").toString());
        setServerPort(map.value("serverPort").toInt());
        setDefaultDatabase(map.value("defaultDatabase").toString());

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
            // here we have leak of 'credential'...
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

    /**
     * @brief Checks whether this connection has primary credential
     * which is also enabled.
     */
    bool ConnectionSettings::hasEnabledPrimaryCredential()
    {
        if (_credentials.count() == 0)
            return false;

        return primaryCredential()->enabled();
    }

    /**
     * @brief Returns primary credential, or NULL if no credentials exists.
     */
    CredentialSettings *ConnectionSettings::primaryCredential()
    {
        if (_credentials.count() == 0)
            return NULL;

        return _credentials.at(0);
    }

    /**
     * @brief Clears and releases memory occupied by credentials
     */
    void ConnectionSettings::clearCredentials()
    {
        qDeleteAll(_credentials);

        _credentials.clear();
        _credentialsByDatabaseName.clear();
    }

}
