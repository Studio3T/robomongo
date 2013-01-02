#ifndef IMPLICITLYSHARED_H
#define IMPLICITLYSHARED_H

#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QSharedDataPointer>
#include <QSharedData>
#include <QDebug>

namespace Robomongo
{

    /**
     * Example of implicitly shared (actually, explicitly) class.
     *
     * You can use it in containers, like QList<ImplicitlyShared> (operator==() specified),
     * and in QHash-like containers (qHash is defined)
     */
    class ImplicitlyShared
    {
    private:

        /**
         * Private data
         */
        class ImplicitlySharedPrivate : public QSharedData
        {
        public:
            ImplicitlySharedPrivate() {}
            int id;
            QString connectionName;
            QString serverHost;
            int serverPort;
            QString userName;
            QString userPassword;
        };

        /**
         * Shared data
         */
        QExplicitlySharedDataPointer<ImplicitlySharedPrivate> _data;
        friend uint qHash(const ImplicitlyShared &key);

    public:

        /**
         * Creates ImplicitlyShared with default values
         */
        ImplicitlyShared();

        /**
         * Shared data support
         */
        ImplicitlyShared(const ImplicitlyShared& other) : _data(other._data) {}
        ImplicitlyShared& operator=(const ImplicitlyShared& other) { _data = other._data; return *this; }
        bool operator ==(const ImplicitlyShared& other) const { return _data == other._data; }
        ~ImplicitlyShared() {}

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
        int id() const { return _data->id; }
        void setId(const int id) { _data->id = id; }

        /**
         * Name of connection
         */
        QString connectionName() const { return _data->connectionName; }
        void setConnectionName(const QString &connectionName) { _data->connectionName = connectionName; }

        /**
         * Database address
         */
        QString serverHost() const { return _data->serverHost; }
        void setServerHost(const QString &serverHost) { _data->serverHost = serverHost; }

        /**
         * Port of database
         */
        int serverPort() const { return _data->serverPort; }
        void setServerPort(const int port) { _data->serverPort = port; }

        /**
         * User name
         */
        QString userName() const { return _data->userName; }
        void setUserName(const QString &userName) { _data->userName = userName; }

        /**
         * Password
         */
        QString userPassword() const { return _data->userPassword; }
        void setUserPassword(const QString &userPassword) { _data->userPassword = userPassword; }

        /**
         * Returns connection full address (i.e. locahost:8090)
         */
        QString getFullAddress() const
        {
            return QString("%1:%2")
                .arg(_data->serverHost)
                .arg(_data->serverPort);
        }
    };

    inline uint qHash(const ImplicitlyShared &key)
    {
        return ::qHash(key._data.data());
    }
}

Q_DECLARE_METATYPE(Robomongo::ImplicitlyShared)




#endif // IMPLICITLYSHARED_H
