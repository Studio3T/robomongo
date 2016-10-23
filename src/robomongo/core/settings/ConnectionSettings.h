#pragma once

#include <QString>
#include <QVariant>
#include <QVariantMap>

#include <mongo/client/dbclientinterface.h>
#include "mongo/client/mongo_uri.h"

#include <boost/algorithm/string.hpp>

namespace Robomongo
{
    class CredentialSettings;
    class SshSettings;
    class SslSettings;
    class ReplicaSetSettings;

    /**
     * @brief Represents connection record
     */
    class ConnectionSettings : public QObject
    {
        Q_OBJECT

    public:
        /**
         * @brief Creates ConnectionSettings with default values
         */
        ConnectionSettings();
        
        /**
        * @brief Creates ConnectionSettings from mongo connection string URI
        */
        ConnectionSettings(const mongo::MongoURI& uri);

        explicit ConnectionSettings(QVariantMap map);

        /**
         * @brief Cleanup used resources
         */
        ~ConnectionSettings();

        /**
         * @brief Creates completely new ConnectionSettings by cloning this record.
         */
        ConnectionSettings *clone() const;

        /**
         * @brief Discards current state and applies state from 'source' ConnectionSettings.
         */
        void apply(const ConnectionSettings *source);

        /**
         * @brief Converts to QVariantMap
         */
        QVariant toVariant() const;
        void fromVariant(const QVariantMap &map);

        /**
         * @brief Name of connection
         */
        std::string connectionName() const { return _connectionName; }
        void setConnectionName(const std::string &connectionName) { _connectionName = connectionName; }

        /**
         * @brief Server host
         */
        std::string serverHost() const { return _host; }
        void setServerHost(const std::string &serverHost) { _host = serverHost; }

        /**
         * @brief Port of server
         */
        unsigned short serverPort() const { return _port; }
        void setServerPort(const int port) { _port = port; }

        /**
         * @brief Default database
         */
        std::string defaultDatabase() const { return _defaultDatabase; }
        void setDefaultDatabase(const std::string &defaultDatabase) { _defaultDatabase = defaultDatabase; }

        /**
         * Was this connection imported from somewhere?
         */
        bool imported() const { return _imported; }
        void setImported(bool imported) { _imported = imported; }

        /**
         * @brief Adds credential to this connection
         */
        void addCredential(CredentialSettings *credential);

        /**
         * @brief Clears and releases memory occupied by credentials
         */
        void clearCredentials();

        /**
         * @brief Checks whether this connection has primary credential
         * which is also enabled.
         */
        bool hasEnabledPrimaryCredential();

        /**
         * @brief Returns primary credential
         */
        CredentialSettings *primaryCredential();

        /**
         * @brief Returns number of credentials
         */
        int credentialCount() const { return _credentials.size(); }

        /**
         * @brief Returns all credentials
         */
        QList<CredentialSettings *> credentials() const { return _credentials; }

        /**
         * @brief Checks that auth required
         */
        /*bool isAuthNeeded() const {
            bool userSpecified = !_userName.isEmpty();
            bool passwordSpecified = !_userPassword.isEmpty();

            return (userSpecified || passwordSpecified);
        }*/

        /**
         * @brief Returns connection full address (i.e. locahost:8090)
         */
        std::string getFullAddress() const;

        std::string getReadableName() const
        {
            if (_connectionName.empty())
                return getFullAddress();

            return _connectionName;
        }

        mongo::HostAndPort info() const {
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

        SshSettings *sshSettings() const { return _sshSettings; }
        SslSettings *sslSettings() const { return _sslSettings; }
        ReplicaSetSettings *replicaSetSettings() const { return _replicaSetSettings; }

        bool isReplicaSet() const { return _isReplicaSet; }
        void setReplicaSet(bool flag) { _isReplicaSet = flag; }

    private:
        CredentialSettings *findCredential(const std::string &databaseName) const;

        std::string _connectionName;
        std::string _host;
        int _port;
        std::string _defaultDatabase;
        QList<CredentialSettings *> _credentials;
        SshSettings *_sshSettings;
        SslSettings *_sslSettings;
        bool _isReplicaSet;
        ReplicaSetSettings *_replicaSetSettings;

        // Was this connection imported from somewhere?
        bool _imported;
    };
}

Q_DECLARE_METATYPE(Robomongo::ConnectionSettings *)
