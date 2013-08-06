#include "robomongo/core/settings/ConnectionSettings.h"

namespace
{
    const unsigned port = 27017;
    const char* defaultServerHost = "localhost";
    const char* defaultNameConnection = "New Connection";
}

namespace Robomongo
{

    /**
     * @brief Creates ConnectionSettings with default values
     */
    ConnectionSettings::ConnectionSettings(): QObject(),
        _serverPort(port),
        _serverHost(defaultServerHost),
        _defaultDatabase(),
        _connectionName(defaultNameConnection) {}

    ConnectionSettings::ConnectionSettings(QVariantMap map) : QObject(),
        _serverPort(map.value("serverPort").toInt()),
        _serverHost(map.value("serverHost").toString()),
        _defaultDatabase(map.value("defaultDatabase").toString()),
        _connectionName(map.value("connectionName").toString()) 
    {
        QVariantList list = map.value("credentials").toList();
        foreach(QVariant var, list) {
            CredentialSettings *credential = new CredentialSettings(var.toMap());
            addCredential(credential);
        }
    }

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
        QList<CredentialSettings *> cred = source->credentials();
        for(QList<CredentialSettings *>::iterator it = cred.begin();it!=cred.end();++it) {
            addCredential((*it)->clone());
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

     CredentialSettings *ConnectionSettings::findCredential(const QString &databaseName)const
     {
         CredentialSettings *result = NULL;
         for(QList<CredentialSettings *>::const_iterator it = _credentials.begin();it!=_credentials.end();++it)
         {
             CredentialSettings * cred = *it;
             if(cred->databaseName()==databaseName){
                 result = cred;
                 break;
             }
         }
         return result;
     }

    /**
     * @brief Adds credential to this connection
     */
    void ConnectionSettings::addCredential(CredentialSettings *credential)
    {
        if (!findCredential(credential->databaseName()))
            _credentials.append(credential);
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
    }

}
