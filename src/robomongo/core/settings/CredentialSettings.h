#pragma once

#include <QString>
#include <QVariant>
#include <QVariantMap>

namespace Robomongo
{
    class CredentialSettings
    {
    public:
        CredentialSettings();
        explicit CredentialSettings(const QVariantMap &map);

        /**
         * @brief Clones credential settings.
         */
        CredentialSettings *clone() const;

        /**
         * @brief Converts to QVariantMap
         */
        QVariant toVariant() const;

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
         * @brief Database name, on which authentication performed
         */
        QString databaseName() const { return _databaseName.isEmpty() ? "admin" : _databaseName; }
        void setDatabaseName(const QString &databaseName) { _databaseName = databaseName; }

        /**
         * @brief Flag, indecating whether we should use this
         * credentials to perform authentication, or not.
         */
        bool enabled() const { return _enabled; }
        void setEnabled(bool enabled) { _enabled = enabled; }

    private:
        QString _userName;
        QString _userPassword;
        QString _databaseName;

        /**
         * @brief Flag, indecating whether we should use this
         * credentials to perform authentication, or not.
         */
        bool _enabled;
    };
}
