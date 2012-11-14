#ifndef CONNECTIONRECORD_H
#define CONNECTIONRECORD_H

#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QSharedDataPointer>
#include <QSharedData>
#include <QDebug>

namespace Robomongo
{

    /*
    ** Represents connection record
    */
    class ConnectionRecord
    {
    private:

        class ConnectionRecordPrivate : public QSharedData
        {
        public:
            ConnectionRecordPrivate() {}
            int id;
            QString connectionName;
            QString databaseAddress;
            int databasePort;
            QString userName;
            QString userPassword;
        };

        QExplicitlySharedDataPointer<ConnectionRecordPrivate> _data;

    public:

        /**
         * Creates ConnectionRecord with default values
         */
        ConnectionRecord();

        ConnectionRecord(const ConnectionRecord& other) : _data(other._data) {}
        ConnectionRecord& operator=(const ConnectionRecord& other) { _data = other._data; return *this; }
        ~ConnectionRecord() {}

        /*
        ** Clone ConnectionRecord
        */
        ConnectionRecord * clone();

        QVariant toVariant() const;
        void fromVariant(QVariantMap map);

        /*
        ** Internal ID of connection
        */
        int id() const { return _data->id; }
        void setId(const int id) { _data->id = id; }

        /*
        ** Name of connection
        */
        QString connectionName() const { return _data->connectionName; }
        void setConnectionName(const QString & connectionName) { _data->connectionName = connectionName; }

        /*
        ** Database address
        */
        QString databaseAddress() const { return _data->databaseAddress; }
        void setDatabaseAddress(const QString & databaseAddress) { _data->databaseAddress = databaseAddress; }

        /*
        ** Port of database
        */
        int databasePort() const { return _data->databasePort; }
        void setDatabasePort(const int port) { _data->databasePort = port; }

        /*
        ** User name
        */
        QString userName() const { return _data->userName; }
        void setUserName(const QString & userName) { _data->userName = userName; }

        /*
        ** Password
        */
        QString userPassword() const { return _data->userPassword; }
        void setUserPassword(const QString & userPassword) { _data->userPassword = userPassword; }

        QString getFullAddress() const
        {
            return QString("%1:%2")
                .arg(_data->databaseAddress)
                .arg(_data->databasePort);
        }
    };
}


#endif // CONNECTIONRECORD_H
