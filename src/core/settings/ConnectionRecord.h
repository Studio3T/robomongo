#ifndef CONNECTIONRECORD_H
#define CONNECTIONRECORD_H

#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QSharedDataPointer>
#include <QSharedData>
#include <QDebug>
#include <Core.h>

namespace Robomongo
{

    /**
     * @brief Represents connection record
     */
    class ConnectionRecord : public QObject
    {
        Q_OBJECT

    public:

        /**
         * @brief Creates ConnectionRecord with default values
         */
        ConnectionRecord();

        /**
         * @brief Creates completely new ConnectionRecord by cloning this record.
         */
        ConnectionRecord *clone() const;

        /**
         * @brief Converts to QVariantMap
         */
        QVariant toVariant() const;

        /**
         * @brief Converts from QVariantMap (and overwrite current state)
         */
        void fromVariant(QVariantMap map);

        /**
         * @brief Internal ID of connection
         */
        int id() const { return _id; }
        void setId(const int id) { _id = id; }

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
         * @brief User name
         */
        QString userName() const { return _userName; }
        void setUserName(const QString &userName) { _userName = userName; }

        /**
         * @brief Password
         */
        QString userPassword() const { return _userPassword; }
        void setUserPassword(const QString &userPassword) { _userPassword = userPassword; }

        /**
         * @brief Port of database
         */
        QString databaseName() const { return _databaseName.isEmpty() ? "test" : _databaseName; }
        void setDatabaseName(const QString &databaseName) { _databaseName = databaseName; }

        /**
         * @brief Checks that auth required
         */
        bool isAuthNeeded() const {
            bool userSpecified = !_userName.isEmpty();
            bool passwordSpecified = !_userPassword.isEmpty();

            return (userSpecified || passwordSpecified);
        }

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

        int _id;
        QString _connectionName;
        QString _databaseAddress;
        int _databasePort;
        QString _userName;
        QString _userPassword;
        QString _databaseName;
    };
}

Q_DECLARE_METATYPE(Robomongo::ConnectionRecord *)

#endif // CONNECTIONRECORD_H
