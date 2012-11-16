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
     * Represents connection record
     */
    class ConnectionRecord : public QObject
    {
        Q_OBJECT

    public:

        /**
         * Creates ConnectionRecord with default values
         */
        ConnectionRecord();

        /**
         * Converts to QVariantMap
         */
        QVariant toVariant() const;

        /**
         * Converts from QVariantMap (and clean current state)
         */
        void fromVariant(QVariantMap map);

        /**
         * Internal ID of connection
         */
        int id() const { return _id; }
        void setId(const int id) { _id = id; }

        /**
         * Name of connection
         */
        QString connectionName() const { return _connectionName; }
        void setConnectionName(const QString & connectionName) { _connectionName = connectionName; }

        /**
         * Database address
         */
        QString databaseAddress() const { return _databaseAddress; }
        void setDatabaseAddress(const QString & databaseAddress) { _databaseAddress = databaseAddress; }

        /**
         * Port of database
         */
        int databasePort() const { return _databasePort; }
        void setDatabasePort(const int port) { _databasePort = port; }

        /**
         * User name
         */
        QString userName() const { return _userName; }
        void setUserName(const QString & userName) { _userName = userName; }

        /**
         * Password
         */
        QString userPassword() const { return _userPassword; }
        void setUserPassword(const QString & userPassword) { _userPassword = userPassword; }

        /**
         * Returns connection full address (i.e. locahost:8090)
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
    };
}

Q_DECLARE_METATYPE(Robomongo::ConnectionRecordPtr)




#endif // CONNECTIONRECORD_H
