#pragma once

#include <QString>
#include <QVariant>
#include <QVariantMap>

#include "robomongo/core/settings/CredentialSettings.h"

namespace Robomongo
{

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
        QString connectionName() const { return _connectionName; }
        void setConnectionName(const QString &connectionName) { _connectionName = connectionName; }

        /**
         * @brief Server host
         */
        QString serverHost() const { return _serverHost; }
        void setServerHost(const QString &serverHost) { _serverHost = serverHost; }

        /**
         * @brief Port of server
         */
        unsigned serverPort() const { return _serverPort; }
        void setServerPort(const int port) { _serverPort = port; }

        /**
         * @brief Default database
         */
        QString defaultDatabase() const { return _defaultDatabase; }
        void setDefaultDatabase(const QString &defaultDatabase) { _defaultDatabase = defaultDatabase; }

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
        int credentialCount() const { return _credentials.count(); }

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
        QString getFullAddress() const
        {
            return QString("%1:%2")
                .arg(_serverHost)
                .arg(_serverPort);
        }

        QString getReadableName() const
        {
            if (_connectionName.isEmpty())
                return getFullAddress();

            return _connectionName;
        }

    private:
        CredentialSettings *findCredential(const QString &databaseName)const;
        QString _connectionName;
        QString _serverHost;
        QString _defaultDatabase;
        unsigned _serverPort;

        QList<CredentialSettings *> _credentials;
    };
}

Q_DECLARE_METATYPE(Robomongo::ConnectionSettings *)
