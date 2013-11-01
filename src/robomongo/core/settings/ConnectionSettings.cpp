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
         _connectionName(defaultNameConnection),
        _info(defaultServerHost,port)
    {

    }

    ConnectionSettings::ConnectionSettings(QVariantMap map) : QObject(),
        _connectionName(QtUtils::toStdString(map.value("connectionName").toString())),
        _info( QtUtils::toStdString(map.value("serverHost").toString()), map.value("serverPort").toInt()
#ifdef MONGO_SSL
        ,SSLInfo(map.value("sslEnabled").toBool(),QtUtils::toStdString(map.value("sslPemKeyFile").toString()))
#endif
#ifdef SSH_SUPPORT_ENABLED
        ,SSHInfo()
#endif        
        )
        ,_defaultDatabase(QtUtils::toStdString(map.value("defaultDatabase").toString()))
    {
#ifdef SSH_SUPPORT_ENABLED
        SSHInfo inf;
        inf._hostName = QtUtils::toStdString(map.value("sshHost").toString());
        inf._userName = QtUtils::toStdString(map.value("sshUserName").toString());
        inf._port = map.value("sshPort").toInt();
        inf._password = QtUtils::toStdString(map.value("sshUserPassword").toString());
        inf._publicKey._publicKey = QtUtils::toStdString(map.value("sshPublicKey").toString());
        inf._publicKey._privateKey = QtUtils::toStdString(map.value("sshPrivateKey").toString());
        inf._publicKey._passphrase = QtUtils::toStdString(map.value("sshPassphrase").toString());
        inf._currentMethod = static_cast<SSHInfo::SupportedAuthenticationMetods>(map.value("sshAuthMethod").toInt());
        setSshInfo(inf);
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
        setSslInfo(source->sslInfo());
#ifdef SSH_SUPPORT_ENABLED
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
#ifdef MONGO_SSL
        SSLInfo infl = _info.sslInfo();
        map.insert("sslEnabled", infl._sslSupport);
        map.insert("sslPemKeyFile", QtUtils::toQString(infl._sslPEMKeyFile));
#endif
#ifdef SSH_SUPPORT_ENABLED
        SSHInfo inf = _info.sshInfo();
        map.insert("sshHost", QtUtils::toQString(inf._hostName));
        map.insert("sshUserName", QtUtils::toQString(inf._userName));
        map.insert("sshPort", inf._port);
        map.insert("sshUserPassword", QtUtils::toQString(inf._password));
        map.insert("sshPublicKey", QtUtils::toQString(inf._publicKey._publicKey));
        map.insert("sshPrivateKey", QtUtils::toQString(inf._publicKey._privateKey));
        map.insert("sshPassphrase", QtUtils::toQString(inf._publicKey._passphrase));
        map.insert("sshAuthMethod", inf._currentMethod);
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
        sprintf(buff, "%s:%u", _info.host().c_str(), _info.port());
        return buff;
    }
}
