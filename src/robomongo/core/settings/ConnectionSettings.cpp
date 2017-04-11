#include "robomongo/core/settings/ConnectionSettings.h"

#include <stdio.h>

#include <QUuid>

#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/settings/ReplicaSetSettings.h"
#include "robomongo/core/settings/SshSettings.h"
#include "robomongo/core/settings/SslSettings.h"
#include "robomongo/core/utils/QtUtils.h"

#include <boost/algorithm/string.hpp>

namespace
{
    const unsigned port = 27017;
    const char *defaultServerHost = "localhost";
    const char *defaultNameConnection = "New Connection";

    const int maxLength = 300;
}

namespace Robomongo
{
    /**
     * Creates ConnectionSettings with default values
     */
    ConnectionSettings::ConnectionSettings(bool isClone) : QObject(),
        _connectionName(defaultNameConnection),
        _host(defaultServerHost),
        _port(port),
        _imported(false),
        _sshSettings(new SshSettings()),
        _sslSettings(new SslSettings()),
        _isReplicaSet(false),
        _replicaSetSettings(new ReplicaSetSettings()),
        _clone(isClone)
    { }

    ConnectionSettings::ConnectionSettings(const mongo::MongoURI& uri, bool isClone)  
        : _connectionName(defaultNameConnection),
        _host(defaultServerHost),
        _port(port),
        _imported(false),
        _sshSettings(new SshSettings()),
        _sslSettings(new SslSettings()),
        _isReplicaSet((uri.type() == mongo::ConnectionString::ConnectionType::SET)),
        _replicaSetSettings(new ReplicaSetSettings(uri)),
        _clone(isClone)
    {
        // Disabling unfinished feature : Export / Import to / from MongoURI
        /*
        if (!uri.getServers().empty()) {
            _host = uri.getServers().front().host();
            _port = uri.getServers().front().port();
        }

        auto str = std::string(uri.getOptions().getStringField("ssl"));
        auto sslEnabled = ("true" == str);
        if (sslEnabled) {
            _sslSettings->enableSSL(true);
            _sslSettings->setAllowInvalidCertificates(true);
        }

        auto credential = new CredentialSettings();
        credential->setUserName(uri.getUser());
        credential->setUserPassword(uri.getPassword());
        credential->setDatabaseName(uri.getDatabase());
        if (!credential->userName().empty() && !credential->userPassword().empty()) {   // todo:
            credential->setEnabled(true);
        }
        addCredential(credential);
        */
    }

    void ConnectionSettings::fromVariant(const QVariantMap &map) {
        setConnectionName(QtUtils::toStdString(map.value("connectionName").toString()));
        setServerHost(QtUtils::toStdString(map.value("serverHost").toString().left(maxLength)));
        setServerPort(map.value("serverPort").toInt());
        setDefaultDatabase(QtUtils::toStdString(map.value("defaultDatabase").toString()));
        setReplicaSet(map.value("isReplicaSet").toBool());       
        
        QVariantList list = map.value("credentials").toList();
        for (QVariantList::const_iterator it = list.begin(); it != list.end(); ++it) {
            QVariant var = *it;
            CredentialSettings *credential = new CredentialSettings(var.toMap());
            addCredential(credential);
        }

        if (map.contains("ssh")) {
            _sshSettings->fromVariant(map.value("ssh").toMap());
        }

        if (map.contains("ssl")) {
            _sslSettings->fromVariant(map.value("ssl").toMap());
        }

        if (isReplicaSet()) {
            _replicaSetSettings->fromVariant(map.value("replicaSet").toMap());
        }

        // If UUID has never been created or is empty, create a new one. Otherwise load the existing.
        if (!map.contains("uuid") || map.value("uuid").toString().isEmpty())
            _uuid = QUuid::createUuid().toString();
        else
            _uuid = map.value("uuid").toString();


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
    }

    /**
     * Creates completely new ConnectionSettings by cloning this record.
     */
    ConnectionSettings *ConnectionSettings::clone() const
    {
        auto settings = new ConnectionSettings(true);
        settings->setUuid(_uuid);
        settings->apply(this);
        return settings;
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
        setImported(source->imported());
        setReplicaSet(source->isReplicaSet());

        clearCredentials();
        QList<CredentialSettings *> cred = source->credentials();
        for (QList<CredentialSettings *>::iterator it = cred.begin(); it != cred.end(); ++it) {
            addCredential((*it)->clone());
        }

        _sshSettings.reset(source->sshSettings()->clone());
        _sslSettings.reset(source->sslSettings()->clone());
        _replicaSetSettings.reset(source->_replicaSetSettings->clone());

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
        map.insert("isReplicaSet", isReplicaSet());
        if (isReplicaSet()) {
            map.insert("replicaSet", _replicaSetSettings->toVariant());
        }

#ifdef MONGO_SSL
        SSLInfo infl = _info.sslInfo();
        map.insert("sslEnabled", infl._sslSupport);
        map.insert("sslPemKeyFile", QtUtils::toQString(infl._sslPEMKeyFile));
#endif
        QVariantList list;
        for (QList<CredentialSettings *>::const_iterator it = _credentials.begin(); it != _credentials.end(); ++it) {
            CredentialSettings *credential = *it;
            list.append(credential->toVariant());
        }
        map.insert("credentials", list);

        map.insert("ssh", _sshSettings->toVariant());
        map.insert("ssl", _sslSettings->toVariant());
        map.insert("uuid", _uuid); 

        return map;
    }

     CredentialSettings *ConnectionSettings::findCredential(const std::string &databaseName) const
     {
         CredentialSettings *result = nullptr;
         for (QList<CredentialSettings *>::const_iterator it = _credentials.begin(); it != _credentials.end(); ++it) {
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
    bool ConnectionSettings::hasEnabledPrimaryCredential() const
    {
        if (_credentials.count() == 0)
            return false;

        return primaryCredential()->enabled();
    }

    /**
     * @brief Returns primary credential
     */
    CredentialSettings *ConnectionSettings::primaryCredential() const
    {
        if (_credentials.count() == 0) {
            _credentials.append(new CredentialSettings());
        }

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
        return hostAndPort().toString();
    }

    mongo::HostAndPort ConnectionSettings::hostAndPort() const 
    {
        // If it doesn't look like IPv6 address,
        // treat it like IPv4 or literal hostname
        if (_host.find(':') == std::string::npos) {
            return mongo::HostAndPort(_host, _port);
        }

        // The following code assumes, that it is IPv6 address
        // If address contains square brackets ("["), remove them:
        std::string hostCopy = _host;
        if (_host.find('[') != std::string::npos) {
            boost::erase_all(hostCopy, "[");
            boost::erase_all(hostCopy, "]");
        }

        return mongo::HostAndPort(hostCopy, _port);
    }

}
