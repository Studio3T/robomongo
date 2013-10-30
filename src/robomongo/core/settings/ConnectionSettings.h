#pragma once

#include <QVariant>
#include <mongo/client/dbclientinterface.h>

#include "robomongo/core/settings/CredentialSettings.h"

namespace Robomongo
{
    class IConnectionSettingsBase
    {
    public:
        enum ConnectionType
        {
            UNKNOWN,
            DIRECT,
            REPLICASET
        };
        IConnectionSettingsBase(ConnectionType connectionType);
        IConnectionSettingsBase(ConnectionType connectionType, const std::string &name, const std::string &defdatabase, const CredentialSettings &cred);
         /**
         * @brief Name of connection
         */
        std::string connectionName() const { return _connectionName; }
        void setConnectionName(const std::string &connectionName) { _connectionName = connectionName; }
        virtual std::string getReadableName() const
        {
            if (_connectionName.empty())
                return getFullAddress();

            return _connectionName;
        }
          /**
         * @brief Default database
         */
        std::string defaultDatabase() const { return _defaultDatabase; }
        void setDefaultDatabase(const std::string &defaultDatabase) { _defaultDatabase = defaultDatabase; }

         /**
         * @brief Returns primary credential, or NULL if no credentials exists.
         */
        void setPrimaryCredential(const CredentialSettings &credential) { _credential = credential; }
        CredentialSettings primaryCredential() const { return _credential; }

        virtual std::string getFullAddress() const = 0;
        virtual QVariant toVariant() const = 0;
        virtual IConnectionSettingsBase *clone() const = 0;
        virtual std::string connectionString() const = 0;
        ConnectionType connectionType() const { return _connectionType; }
    protected:
        std::string _connectionName;
        std::string _defaultDatabase;
        CredentialSettings _credential;
        ConnectionType _connectionType;
    };

    /**
     * @brief Represents connection record
     */
    class ConnectionSettings : public IConnectionSettingsBase
    {
    public:
        typedef mongo::HostAndPort hostInfoType;
        /**
         * @brief Creates ConnectionSettings with default values
         */
        ConnectionSettings();

        explicit ConnectionSettings(QVariantMap map);

        virtual ~ConnectionSettings(){}
        /**
         * @brief Converts to QVariant
         */
        virtual QVariant toVariant() const;

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
         * @brief Returns connection full address (i.e. locahost:8090)
         */
        virtual std::string getFullAddress() const;

        virtual IConnectionSettingsBase *clone() const;
        virtual std::string connectionString() const;

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
        hostInfoType _info;
    };

    
    class ReplicasetConnectionSettings : public IConnectionSettingsBase
    {
    public:
        typedef std::vector<ConnectionSettings *> ServerContainerType;
        
        ReplicasetConnectionSettings();
        explicit ReplicasetConnectionSettings(QVariantMap map);

        virtual std::string getFullAddress() const;
        virtual QVariant toVariant() const;
        virtual IConnectionSettingsBase *clone() const;

        ServerContainerType servers() const { return _servers; }
        void addServer(ConnectionSettings *server);
        void removeServer(ConnectionSettings *server);
        void clearServers();
        virtual std::string connectionString() const;

        std::string replicaName() const { return _replicaName; }
        void setReplicaName(const std::string &name) { _replicaName = name; }

        std::vector<ConnectionSettings::hostInfoType> serversHostsInfo() const;
    private:
        ServerContainerType _servers;
        std::string _replicaName;
    };
}

Q_DECLARE_METATYPE(Robomongo::IConnectionSettingsBase *)
