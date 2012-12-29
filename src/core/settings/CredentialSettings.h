#ifndef CREDENTIALSETTINGS_H
#define CREDENTIALSETTINGS_H

#include <QString>
#include <QVariant>
#include <QVariantMap>

namespace Robomongo
{
    class CredentialSettings
    {
    public:

        /**
         * @brief Creates CredentialSettings.
         * @param userName
         * @param userPassword
         * @param databaseName: name of databases, on which authentication is performed.
         */
        CredentialSettings(const QString &userName,
                           const QString &userPassword,
                           const QString &databaseName = "admin");

        CredentialSettings(const QVariantMap &map);

        /**
         * @brief Clones credential settings.
         */
        CredentialSettings *clone() const;

        /**
         * @brief Converts to QVariantMap
         */
        QVariant toVariant() const;

        /**
         * @brief Converts from QVariantMap (and overwrite current state)
         */
        void fromVariant(QVariantMap map);


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

    private:
        QString _userName;
        QString _userPassword;
        QString _databaseName;
    };
}


#endif // CREDENTIALSETTINGS_H
