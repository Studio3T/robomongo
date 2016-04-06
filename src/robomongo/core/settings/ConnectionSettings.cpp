#include "robomongo/core/settings/ConnectionSettings.h"

#include <stdio.h>

#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/settings/SshSettings.h"
#include "robomongo/core/utils/QtUtils.h"

namespace
{
    const unsigned port = 27017;
    const char *defaultServerHost = "localhost";
    const char *defaultNameConnection = "New Connection";
}

namespace Robomongo
{

    /**
     * Creates ConnectionSettings with default values
     */
    ConnectionSettings::ConnectionSettings() : QObject(),
        _connectionName(defaultNameConnection),
        _host(defaultServerHost),
        _port(port),
        _sshSettings(new SshSettings()) { }

    void ConnectionSettings::fromVariant(const QVariantMap &map) {
        setConnectionName(QtUtils::toStdString(map.value("connectionName").toString()));
        setServerHost(QtUtils::toStdString(map.value("serverHost").toString()));
        setServerPort(map.value("serverPort").toInt());
        setDefaultDatabase(QtUtils::toStdString(map.value("defaultDatabase").toString()));

        QVariantList list = map.value("credentials").toList();
        for(QVariantList::const_iterator it = list.begin(); it != list.end(); ++it) {
            QVariant var = *it;
            CredentialSettings *credential = new CredentialSettings(var.toMap());
            addCredential(credential);
        }

        if (map.contains("ssh")) {
            _sshSettings->fromVariant(map.value("ssh").toMap());
        }

//#ifdef MONGO_SSL
//      ,SSLInfo(map.value("sslEnabled").toBool(),QtUtils::toStdString(map.value("sslPemKeyFile").toString()))
//#endif
    }

    /**
     * Cleanup used resources
     */
    ConnectionSettings::~ConnectionSettings()
    {
        clearCredentials();
        delete _sshSettings;
    }

    /**
     * Creates completely new ConnectionSettings by cloning this record.
     */
    ConnectionSettings *ConnectionSettings::clone() const
    {
        ConnectionSettings *record = new ConnectionSettings();
        record->apply(this);
        return record;
    }

    /**
     * Discards current state and applies state from 'source' ConnectionSettings.
     */
    void ConnectionSettings::apply(const ConnectionSettings *source)
    {
        setConnectionName(source->connectionName());
        setServerHost(source->serverHost());
        setServerPort(source->serverPort());
        setDefaultDatabase(source->defaultDatabase());

        clearCredentials();
        QList<CredentialSettings *> cred = source->credentials();
        for (QList<CredentialSettings *>::iterator it = cred.begin(); it != cred.end(); ++it) {
            addCredential((*it)->clone());
        }

        // Clone SSH settings
        _sshSettings = source->sshSettings()->clone();

//#ifdef MONGO_SSL
//        setSslInfo(source->sslInfo());
//#endif
    }

    /**
     * @brief Converts to QVariantMap
     */
    QVariant ConnectionSettings::toVariant() const
    {
        QVariantMap map;
        map.insert("connectionName", QtUtils::toQString(connectionName()));
        map.insert("serverHost", QtUtils::toQString(serverHost()));
        map.insert("serverPort", serverPort());
        map.insert("defaultDatabase", QtUtils::toQString(defaultDatabase()));
#ifdef MONGO_SSL
        SSLInfo infl = _info.sslInfo();
        map.insert("sslEnabled", infl._sslSupport);
        map.insert("sslPemKeyFile", QtUtils::toQString(infl._sslPEMKeyFile));
#endif
        QVariantList list;
        for(QList<CredentialSettings *>::const_iterator it = _credentials.begin(); it != _credentials.end(); ++it) {
            CredentialSettings *credential = *it;
            list.append(credential->toVariant());
        }
        map.insert("credentials", list);

        map.insert("ssh", _sshSettings->toVariant());

        return map;
    }

     CredentialSettings *ConnectionSettings::findCredential(const std::string &databaseName) const
     {
         CredentialSettings *result = NULL;
         for(QList<CredentialSettings *>::const_iterator it = _credentials.begin(); it != _credentials.end(); ++it) {
             CredentialSettings *cred = *it;
             if (cred->databaseName() == databaseName) {
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

    std::string ConnectionSettings::getFullAddress() const
    {
        return info().toString();
    }



}
