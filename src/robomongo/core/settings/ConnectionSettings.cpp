#include "robomongo/core/settings/ConnectionSettings.h"

#include <stdio.h>

#include "robomongo/core/utils/QtUtils.h"

#define CONNECTIONNAME "connectionName"
#define SSLSUPPORT "ssl"
#define SSLPEMKEYFILE "sslPEMKeyFile"
#define SERVERHOST "serverHost"
#define SERVERPORT "serverPort"
#define DEFAULTDATABASE "defaultDatabase"
#define SSHINFO_HOST "sshInfo.host"
#define SSHINFO_USERNAME "sshInfo.username"
#define SSHINFO_PORT "sshInfo.port"
#define SSHINFO_PASSWORD "sshInfo.password"
#define SSHINFO_PUBLICKEY "sshInfo.publicKey"
#define SSHINFO_PRIVATEKEY "sshInfo.privateKey"
#define SSHINFO_PASSPHARASE "sshInfo.passphrase"
#define SSHINFO_AUTHMETHOD "sshInfo.authMethod"
#define CREDENTIALS "credentials"

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
    ConnectionSettings::ConnectionSettings() :
         _connectionName(defaultNameConnection),
        _info(defaultServerHost,port)
    {

    }

    ConnectionSettings::ConnectionSettings(QVariantMap map) :
        _connectionName(QtUtils::toStdString(map.value(CONNECTIONNAME).toString()))
        ,_defaultDatabase(QtUtils::toStdString(map.value(DEFAULTDATABASE).toString())),
        _info( QtUtils::toStdString(map.value(SERVERHOST).toString()), map.value(SERVERPORT).toInt()
#ifdef MONGO_SSL
        ,SSLInfo(map.value(SSLSUPPORT).toBool(),QtUtils::toStdString(map.value(SSLPEMKEYFILE).toString()))
#endif
#ifdef SSH_SUPPORT_ENABLED
        ,SSHInfo()
#endif        
        )
    {
#ifdef SSH_SUPPORT_ENABLED
        SSHInfo inf;
        inf._hostName = QtUtils::toStdString(map.value(SSHINFO_HOST).toString());
        inf._userName = QtUtils::toStdString(map.value(SSHINFO_USERNAME).toString()); 
        inf._port = map.value(SSHINFO_PORT).toInt();
        inf._password = QtUtils::toStdString(map.value(SSHINFO_PASSWORD).toString());
        inf._publicKey._publicKey = QtUtils::toStdString(map.value(SSHINFO_PUBLICKEY).toString());
        inf._publicKey._privateKey = QtUtils::toStdString(map.value(SSHINFO_PRIVATEKEY).toString());
        inf._publicKey._passphrase = QtUtils::toStdString(map.value(SSHINFO_PASSPHARASE).toString());
        inf._currentMethod = static_cast<SSHInfo::SupportedAuthenticationMetods>(map.value(SSHINFO_AUTHMETHOD).toInt());
        _info.setSshInfo(inf);
#endif
        //delete this after some releases
        QList<QVariant> oldVersion = map.value(CREDENTIALS).toList();
        if(oldVersion.count()==1){
            CredentialSettings credential(oldVersion[0].toMap());
            setPrimaryCredential(credential);
        }
        else{
            CredentialSettings credential(map.value(CREDENTIALS).toMap());
            setPrimaryCredential(credential);
    }
    }

    ConnectionSettings::ConnectionSettings(const ConnectionSettings &old)
    {
        setConnectionName(old.connectionName());
        setServerHost(old.serverHost());
        setServerPort(old.serverPort());
        setDefaultDatabase(old.defaultDatabase());
#ifdef MONGO_SSL
        _info.setSslInfo(old.sslInfo());
#endif
#ifdef SSH_SUPPORT_ENABLED
        _info.setSshInfo(old.sshInfo());
#endif
        setPrimaryCredential(old._credential);
    }

    /**
     * @brief Converts to QVariantMap
     */
    QVariant ConnectionSettings::toVariant() const
    {
        QVariantMap map;
        map.insert(CONNECTIONNAME, QtUtils::toQString(connectionName()));
        map.insert(SERVERHOST, QtUtils::toQString(serverHost()));
        map.insert(SERVERPORT, serverPort());
        map.insert(DEFAULTDATABASE, QtUtils::toQString(defaultDatabase()));
#ifdef MONGO_SSL
        SSLInfo infl = _info.sslInfo();
        map.insert(SSLSUPPORT, infl._sslSupport);
        map.insert(SSLPEMKEYFILE, QtUtils::toQString(infl._sslPEMKeyFile));
#endif
#ifdef SSH_SUPPORT_ENABLED
        SSHInfo inf = _info.sshInfo();
        map.insert(SSHINFO_HOST, QtUtils::toQString(inf._hostName));
        map.insert(SSHINFO_USERNAME, QtUtils::toQString(inf._userName));
        map.insert(SSHINFO_PORT, inf._port);
        map.insert(SSHINFO_PASSWORD, QtUtils::toQString(inf._password));
        map.insert(SSHINFO_PUBLICKEY, QtUtils::toQString(inf._publicKey._publicKey));
        map.insert(SSHINFO_PRIVATEKEY, QtUtils::toQString(inf._publicKey._privateKey));
        map.insert(SSHINFO_PASSPHARASE, QtUtils::toQString(inf._publicKey._passphrase));
        map.insert(SSHINFO_AUTHMETHOD, inf._currentMethod);
#endif
        map.insert(CREDENTIALS, _credential.toVariant());

        return map;
    }

    /**
     * @brief Adds credential to this connection
     */
    void ConnectionSettings::setPrimaryCredential(const CredentialSettings &credential)
    {
        _credential = credential;
    }

    /**
     * @brief Returns primary credential, or NULL if no credentials exists.
     */
    CredentialSettings ConnectionSettings::primaryCredential() const
    {
        return _credential;
    }

    std::string ConnectionSettings::getFullAddress() const
    {
        char buff[1024] = {0};
        sprintf(buff, "%s:%u", _info.host().c_str(), _info.port());
        return buff;
    }
}
