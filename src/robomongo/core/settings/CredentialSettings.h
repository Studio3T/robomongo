#pragma once

#include <QVariantMap>

namespace Robomongo
{
    class CredentialSettings
    {
    public:
        struct CredentialInfo
        {
            CredentialInfo();
            CredentialInfo(const std::string &userName, const std::string &userPassword, const std::string &databaseName);
            bool isValid() const { return !_userName.empty() && !_databaseName.empty(); }
            std::string _userName;
            std::string _userPassword;
            std::string _databaseName;
        };

        CredentialSettings();
        explicit CredentialSettings(const QVariantMap &map);

        /**
         * @brief Converts to QVariantMap
         */
        QVariantMap toVariant() const;

        bool isValidAndEnabled() const { return _credentialInfo.isValid() && _enabled; }
        /**
         * @brief Flag, indecating whether we should use this
         * credentials to perform authentication, or not.
         */
        bool enabled() const { return _enabled; }
        void setEnabled(bool enabled) { _enabled = enabled; }

        CredentialInfo info() const { return _credentialInfo; }
        void setInfo(const CredentialInfo &info) { _credentialInfo = info; }

    private:
        CredentialInfo _credentialInfo;
        /**
         * @brief Flag, indecating whether we should use this
         * credentials to perform authentication, or not.
         */
        bool _enabled;
    };

    inline bool operator==(const CredentialSettings::CredentialInfo& r,const CredentialSettings::CredentialInfo& l) 
    { 
        return r._databaseName == l._databaseName && r._userName == l._userName && r._userPassword == l._userPassword;
    }
}
