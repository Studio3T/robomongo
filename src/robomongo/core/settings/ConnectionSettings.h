#pragma once

#include <QVariant>
#include <mongo/client/dbclientinterface.h>

#include "robomongo/core/settings/CredentialSettings.h"

namespace Robomongo
{
    /**
     * @brief Represents connection record
     */
    class ConnectionSettings
    {
    public:
        typedef mongo::HostAndPort hostInfoType;
        /**
         * @brief Creates ConnectionSettings with default values
         */
        ConnectionSettings();

        explicit ConnectionSettings(QVariantMap map);
        ConnectionSettings(const ConnectionSettings &old);

        /**
         * @brief Converts to QVariant
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
         * @brief Returns primary credential, or NULL if no credentials exists.
         */
        void setPrimaryCredential(const CredentialSettings &credential);
        CredentialSettings primaryCredential() const;

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

        hostInfoType info() const { return _info; }
#ifdef MONGO_SSL
        SSLInfo sslInfo() const {return _info.sslInfo(); }
        void setSslInfo(const SSLInfo &info) {_info.setSslInfo(info);}
#endif // MONGO_SSL
#ifdef SSH_SUPPORT_ENABLED
       SSHInfo sshInfo() const {return _info.sshInfo(); }
       void setSshInfo(const SSHInfo &info) {_info.setSshInfo(info);}
#endif // SSH_SUPPORT_ENABLED

    private:
        std::string _connectionName;
        std::string _defaultDatabase;
        CredentialSettings _credential;

        hostInfoType _info;
    };
}

Q_DECLARE_METATYPE(Robomongo::ConnectionSettings *)
