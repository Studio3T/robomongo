#include "robomongo/core/settings/ConnectionSettings.h"

#include <stdio.h>

#include "robomongo/core/settings/CredentialSettings.h"
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
     * @brief Creates ConnectionSettings with default values
     */
    ConnectionSettings::ConnectionSettings() : QObject(),
        _serverPort(port),
        _serverHost(defaultServerHost),
        _defaultDatabase(),
        _connectionName(defaultNameConnection),
        _sslSupport(false),
        _sslPEMKeyFile(){}

    ConnectionSettings::ConnectionSettings(QVariantMap map) : QObject(),
        _serverPort(map.value("serverPort").toInt()),
        _serverHost(QtUtils::toStdString(map.value("serverHost").toString())),
        _defaultDatabase(QtUtils::toStdString(map.value("defaultDatabase").toString())),
        _connectionName(QtUtils::toStdString(map.value("connectionName").toString())),
        _sslSupport(map.value("ssl").toBool()),
        _sslPEMKeyFile(QtUtils::toStdString(map.value("sslPEMKeyFile").toString()))
    {
#ifdef OPENSSH_SUPPORT_ENABLED
        _sshInfo._hostName = QtUtils::toStdString(map.value("sshInfo.host").toString());
        _sshInfo._userName = QtUtils::toStdString(map.value("sshInfo.username").toString()); 
        _sshInfo._port = map.value("sshInfo.port").toInt();
        _sshInfo._password = QtUtils::toStdString(map.value("sshInfo.password").toString());
#endif
        QVariantList list = map.value("credentials").toList();
        for(QVariantList::const_iterator it = list.begin(); it != list.end(); ++it) {
            QVariant var = *it;
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
        setSslSupport(source->isSslSupport());
        setSslPEMKeyFile(source->sslPEMKeyFile());
#ifdef OPENSSH_SUPPORT_ENABLED
        setSshInfo(source->sshInfo());
#endif
        clearCredentials();
        QList<CredentialSettings *> cred = source->credentials();
        for (QList<CredentialSettings *>::iterator it = cred.begin(); it != cred.end(); ++it) {
            addCredential((*it)->clone());
        }
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
        map.insert("ssl", _sslSupport);
        map.insert("sslPEMKeyFile", QtUtils::toQString(sslPEMKeyFile()));
#ifdef OPENSSH_SUPPORT_ENABLED
        map.insert("sshInfo.host", QtUtils::toQString(_sshInfo._hostName));
        map.insert("sshInfo.username", QtUtils::toQString(_sshInfo._userName));
        map.insert("sshInfo.port", _sshInfo._port);
        map.insert("sshInfo.password", QtUtils::toQString(_sshInfo._password));
#endif
        QVariantList list;
        for(QList<CredentialSettings *>::const_iterator it = _credentials.begin(); it != _credentials.end(); ++it) {
            CredentialSettings *credential = *it;
            list.append(credential->toVariant());
        }
        map.insert("credentials", list);

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
        char buff[1024] = {0};
        sprintf(buff, "%s:%u", _serverHost.c_str(), _serverPort);
        return buff;
    }
}
