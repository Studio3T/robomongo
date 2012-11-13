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

    /*
    ** Represents connection record
    */
    class ConnectionRecord
    {
    private:
        QExplicitlySharedDataPointer<ConnectionRecordPrivate> _this;

    public:

        /**
         * Creates ConnectionRecord with default values
         */
        ConnectionRecord();

        ConnectionRecord(const ConnectionRecord& other) : _this(other._this) {}
        ConnectionRecord& operator=(const ConnectionRecord& other) { _this = other._this; return *this; }
        ~ConnectionRecord() { qDebug() << "Distructed"; }

        /*
        ** Clone ConnectionRecord
        */
        ConnectionRecord * clone();

        QVariant toVariant() const;
        void fromVariant(QVariantMap map);

        /*
        ** Internal ID of connection
        */
        int id() const { return _this->id; }
        void setId(const int id) { _this->id = id; }

        /*
        ** Name of connection
        */
        QString connectionName() const { return _this->connectionName; }
        void setConnectionName(const QString & connectionName) { _this->connectionName = connectionName; }

        /*
        ** Database address
        */
        QString databaseAddress() const { return _this->databaseAddress; }
        void setDatabaseAddress(const QString & databaseAddress) { _this->databaseAddress = databaseAddress; }

        /*
        ** Port of database
        */
        int databasePort() const { return _this->databasePort; }
        void setDatabasePort(const int port) { _this->databasePort = port; }

        /*
        ** User name
        */
        QString userName() const { return _this->userName; }
        void setUserName(const QString & userName) { _this->userName = userName; }

        /*
        ** Password
        */
        QString userPassword() const { return _this->userPassword; }
        void setUserPassword(const QString & userPassword) { _this->userPassword = userPassword; }

        QString getFullAddress() const
        {
            return QString("%1:%2")
                .arg(_this->databaseAddress)
                .arg(_this->databasePort);
        }
    };
}


#endif // CONNECTIONRECORD_H
