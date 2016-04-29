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
        std::string userName() const { return _userName; }
        void setUserName(const std::string &userName) { _userName = userName; }

        /**
         * @brief Password
         */
        std::string userPassword() const { return _userPassword; }
        void setUserPassword(const std::string &userPassword) { _userPassword = userPassword; }

        /**
         * @brief Database name, on which authentication performed
         */
        std::string databaseName() const { return _databaseName.empty() ? "admin" : _databaseName; }
        void setDatabaseName(const std::string &databaseName) { _databaseName = databaseName; }

        /**
         * @brief Authentication mechanism (SCRAM-SHA-1 or MONGODB-CR)
         */
        std::string mechanism() const { return _mechanism.empty() ? "SCRAM-SHA-1" : _mechanism; }
        void setMechanism(const std::string &mechanism) { _mechanism = mechanism; }

        /**
         * @brief Flag, indecating whether we should use this
         * credentials to perform authentication, or not.
         */
        bool enabled() const { return _enabled; }
        void setEnabled(bool enabled) { _enabled = enabled; }

    private:
        std::string _userName;
        std::string _userPassword;
        std::string _databaseName;
        std::string _mechanism; // authentication mechanism (MONGODB-CR or SCRAM-SHA-1)

        /**
         * @brief Flag, indecating whether we should use this
         * credentials to perform authentication, or not.
         */
        bool _enabled;
    };
}
