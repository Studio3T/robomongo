#ifndef CONNECTIONRECORD_H
#define CONNECTIONRECORD_H

#include <QString>
#include <QVariant>
#include <QVariantMap>

namespace Robomongo
{

    /*
    ** Represents connection record
    */
    class ConnectionRecord
    {

    private:
        /*
        ** Internal ID of connection
        */
        int _id;

        /*
        ** Name of connection
        */
        QString _connectionName;

        /*
        ** Database address
        */
        QString _databaseAddress;

        /*
        ** Port of database
        */
        int _databasePort;

        /*
        ** User name
        */
        QString _userName;

        /*
        ** Password
        */
        QString _userPassword;

    public:
        /*
        ** Constructs connection record
        */
        ConnectionRecord();

        /*
        ** Destructs connection record
        */
        ~ConnectionRecord();

        /*
        ** Clone ConnectionRecord
        */
        ConnectionRecord * clone();

        QVariant toVariant() const;
        void fromVariant(QVariantMap map);

        /*
        ** Internal ID of connection
        */
        int id() const { return _id; }
        void setId(const int id) { _id = id; }

        /*
        ** Name of connection
        */
        QString connectionName() const { return _connectionName; }
        void setConnectionName(const QString & connectionName) { _connectionName = connectionName; }

        /*
        ** Database address
        */
        QString databaseAddress() const { return _databaseAddress; }
        void setDatabaseAddress(const QString & databaseAddress) { _databaseAddress = databaseAddress; }

        /*
        ** Port of database
        */
        int databasePort() const { return _databasePort; }
        void setDatabasePort(const int port) { _databasePort = port; }

        /*
        ** User name
        */
        QString userName() const { return _userName; }
        void setUserName(const QString & userName) { _userName = userName; }

        /*
        ** Password
        */
        QString userPassword() const { return _userPassword; }
        void setUserPassword(const QString & userPassword) { _userPassword = userPassword; }

        QString getFullAddress() const
        {
            return QString("%1:%2")
                .arg(_databaseAddress)
                .arg(_databasePort);
        }
    };
}


#endif // CONNECTIONRECORD_H
