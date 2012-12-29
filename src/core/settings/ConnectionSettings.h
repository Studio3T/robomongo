#ifndef CONNECTIONRECORD_H
#define CONNECTIONRECORD_H

#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QSharedDataPointer>
#include <QSharedData>
#include <QDebug>
#include <Core.h>
#include "CredentialSettings.h"

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

        /**
         * @brief Creates completely new ConnectionSettings by cloning this record.
         */
        ConnectionSettings *clone() const;

        /**
         * @brief Converts to QVariantMap
         */
        QVariant toVariant() const;

        /**
         * @brief Converts from QVariantMap (and overwrite current state)
         */
        void fromVariant(QVariantMap map);

        /**
         * @brief Name of connection
         */
        QString connectionName() const { return _connectionName; }
        void setConnectionName(const QString &connectionName) { _connectionName = connectionName; }

        /**
         * @brief Database address
         */
        QString databaseAddress() const { return _databaseAddress; }
        void setDatabaseAddress(const QString &databaseAddress) { _databaseAddress = databaseAddress; }

        /**
         * @brief Port of database
         */
        int databasePort() const { return _databasePort; }
        void setDatabasePort(const int port) { _databasePort = port; }

        /**
         * @brief Adds credential to this connection
         */
        void addCredential(CredentialSettings *credential);

        /**
         * @brief Returns credential for specified database, or NULL if no such
         * credential in connection.
         */
        CredentialSettings *credential(QString databaseName);
        CredentialSettings *credential(int index);
        int credentialCount() const { return _credentials.count(); }

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
                .arg(_databaseAddress)
                .arg(_databasePort);
        }

        QString getReadableName() const
        {
            if (_connectionName.isEmpty())
                return getFullAddress();

            return _connectionName;
        }

    private:

        QString _connectionName;
        QString _databaseAddress;
        QString _defaultDatabase;
        int _databasePort;

        QList<CredentialSettings *> _credentials;
        QHash<QString, CredentialSettings *> _credentialsByDatabaseName;
    };
}

Q_DECLARE_METATYPE(Robomongo::ConnectionSettings *)

#endif // CONNECTIONRECORD_H
