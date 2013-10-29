#include "robomongo/core/settings/ConnectionSettings.h"

#include <stdio.h>

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/utils/StdUtils.h"

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

#define REPLICASETSERVERS "replicasetservers"
#define REPLICASETNAME "replicasetname"

namespace
{
    const unsigned port = 27017;
    const char *defaultServerHost = "localhost";
    const char *defaultNameConnection = "New Connection";
}

namespace Robomongo
{
    IConnectionSettingsBase::IConnectionSettingsBase():
                 _connectionName(defaultNameConnection), _defaultDatabase(), _credential()
    {

    }

    IConnectionSettingsBase::IConnectionSettingsBase(const std::string &name, const std::string &defdatabase, const CredentialSettings &cred):
        _connectionName(name), _defaultDatabase(defdatabase), _credential(cred)
    {

    }
    /**
     * @brief Creates ConnectionSettings with default values
     */
    ConnectionSettings::ConnectionSettings() :
        IConnectionSettingsBase(),
        _info(defaultServerHost,port)
    {

    }

    ConnectionSettings::ConnectionSettings(QVariantMap map) :
        IConnectionSettingsBase(QtUtils::toStdString(map.value(CONNECTIONNAME).toString()), QtUtils::toStdString(map.value(DEFAULTDATABASE).toString()), CredentialSettings() )
        ,_info( QtUtils::toStdString(map.value(SERVERHOST).toString()), map.value(SERVERPORT).toInt()
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

    IConnectionSettingsBase *ConnectionSettings::clone() const
    {
        ConnectionSettings *set = new ConnectionSettings(*this);
        return set;
    }

    std::string ConnectionSettings::connectionString() const
    {
        std::stringstream ss;
        ss << serverHost() << ":" << serverPort() 
#ifdef MONGO_SSL
            << sslInfo()
#endif
#ifdef SSH_SUPPORT_ENABLED
            << sshInfo()
#endif
            ;
        return ss.str();
    }

    ReplicasetConnectionSettings::ReplicasetConnectionSettings() :
        IConnectionSettingsBase()
    {
    }

    ReplicasetConnectionSettings::ReplicasetConnectionSettings(QVariantMap map):
        IConnectionSettingsBase(QtUtils::toStdString(map.value(CONNECTIONNAME).toString()), QtUtils::toStdString(map.value(DEFAULTDATABASE).toString()), CredentialSettings() ),
        _replicaName(QtUtils::toStdString(map.value(REPLICASETNAME).toString()))
    {
        CredentialSettings credential(map.value(CREDENTIALS).toMap());
        setPrimaryCredential(credential);
        QList<QVariant> servers = map.value(REPLICASETSERVERS).toList();
        for (QList<QVariant>::const_iterator it = servers.begin(); it!= servers.end(); ++it)
        {
            QVariant var = *it;
            addServer(new ConnectionSettings(var.toMap()));
        }
    }

    std::string ConnectionSettings::getFullAddress() const
    {
        char buff[1024] = {0};
        sprintf(buff, "%s:%u", _info.host().c_str(), _info.port());
        return buff;
    }

    std::string ReplicasetConnectionSettings::getFullAddress() const
    {
        std::string res;
        for (ServerContainerType::const_iterator it = _servers.begin(); it!= _servers.end(); ++it)
        {
            ConnectionSettings *ser = *it;
            res += ser->getFullAddress();
        }
        return res;
    }

    QVariant ReplicasetConnectionSettings::toVariant() const
    {
        QVariantMap map;
        map.insert(CONNECTIONNAME, QtUtils::toQString(connectionName()));
        map.insert(DEFAULTDATABASE, QtUtils::toQString(defaultDatabase()));
        map.insert(REPLICASETNAME, QtUtils::toQString(replicaName()));
        map.insert(CREDENTIALS, _credential.toVariant());
        QVariantList listS;
        for (ServerContainerType::const_iterator it = _servers.begin(); it!= _servers.end(); ++it)
        {
            ConnectionSettings *ser = *it;
            listS.append(ser->toVariant());
        }
        map.insert(REPLICASETSERVERS,listS);
        return map;
    }

    IConnectionSettingsBase *ReplicasetConnectionSettings::clone() const
    {
        ReplicasetConnectionSettings *set = new ReplicasetConnectionSettings(*this);
        return set;
    }

    void ReplicasetConnectionSettings::addServer(ConnectionSettings *server)
    {
        _servers.push_back(server);
    }

    void ReplicasetConnectionSettings::removeServer(ConnectionSettings *server)
    {
        _servers.erase(std::remove_if(_servers.begin(), _servers.end(), stdutils::RemoveIfFound<ConnectionSettings*>(server)),_servers.end());
    }

    std::string ReplicasetConnectionSettings::connectionString() const
    {
        std::string result;
        int count = _servers.size();
        for (int i = 0; i<count; ++i )
        {
            result+=_servers[i]->connectionString();
            if(i<count-1){
                result+=',';
            }
        }
        return result;
    }

    std::vector<ConnectionSettings::hostInfoType> ReplicasetConnectionSettings::serversHostsInfo() const
    {
        std::vector<ConnectionSettings::hostInfoType> result;
        int count = _servers.size();
        for (int i = 0; i<count; ++i)
        {
            result.push_back(_servers[i]->info());
        }
        return result;
    }
}
