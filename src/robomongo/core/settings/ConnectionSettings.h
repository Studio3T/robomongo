#pragma once

#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <mongo/client/dbclientinterface.h>

namespace Robomongo
{
    class CredentialSettings;
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

        /**
         * @brief Name of connection
         */
        std::string connectionName() const { return _connectionName; }
        void setConnectionName(const std::string &connectionName) { _connectionName = connectionName; }

        /**
         * @brief Server host
         */
        std::string serverHost() const { return _info.host(); }
        void setServerHost(const std::string &serverHost) { _info.setHost(serverHost); }

        /**
         * @brief Port of server
         */
        unsigned short serverPort() const { return _info.port(); }
        void setServerPort(const int port) { _info.setPort(port); }

        /**
         * @brief Default database
         */
        std::string defaultDatabase() const { return _defaultDatabase; }
        void setDefaultDatabase(const std::string &defaultDatabase) { _defaultDatabase = defaultDatabase; }

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
         * @brief Returns primary credential, or NULL if no credentials exists.
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

        mongo::HostAndPort info() const {return _info;}
#ifdef MONGO_SSL
        SSLInfo sslInfo() const {return _info.sslInfo(); }
        void setSslInfo(const SSLInfo &info) {_info.setSslInfo(info);}
#endif // MONGO_SSL
#ifdef SSH_SUPPORT_ENABLED
       SSHInfo sshInfo() const {return _info.sshInfo(); }
       void setSshInfo(const SSHInfo &info) {_info.setSshInfo(info);}
#endif // SSH_SUPPORT_ENABLED

    private:
        CredentialSettings *findCredential(const std::string &databaseName) const;
        std::string _connectionName;
        mongo::HostAndPort _info;
        std::string _defaultDatabase;
        QList<CredentialSettings *> _credentials;
    };
}

Q_DECLARE_METATYPE(Robomongo::ConnectionSettings *)
