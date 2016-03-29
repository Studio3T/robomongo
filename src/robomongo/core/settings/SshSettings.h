#pragma once

#include <QString>
#include <QVariant>
#include <QVariantMap>

namespace Robomongo
{
    class SshSettings
    {
    public:
        SshSettings();

        /**
         * Clones credential settings.
         */
        SshSettings *clone() const;

        /**
         * Converts to QVariantMap
         */
        QVariant toVariant() const;
        void fromVariant(const QVariantMap &map);

        std::string host() const { return _host; }
        void setHost(const std::string &host) { _host = host; }

        int port() const { return _port; }
        void setPort(const int port) { _port = port; }

        std::string userName() const { return _userName; }
        void setUserName(const std::string &userName) { _userName = userName; }

        std::string userPassword() const { return _userPassword; }
        void setUserPassword(const std::string &userPassword) { _userPassword = userPassword; }

        std::string privateKeyFile() const { return _privateKeyFile; }
        void setPrivateKeyFile(const std::string &path) { _privateKeyFile = path; }

        std::string publicKeyFile() const { return _publicKeyFile; }
        void setPublicKeyFile(const std::string &path) { _publicKeyFile = path; }

        std::string passphrase() const { return _passphrase; }
        void setPassphrase(const std::string &passphrase) { _passphrase = passphrase; }

        std::string authMethod() const { return _authMethod.empty() ? "publickey" : _authMethod; }
        void setAuthMethod(const std::string &method) { _authMethod = method; }

        /**
         * @brief Flag, indecating whether we should use this
         * credentials to perform authentication, or not.
         */
        bool enabled() const { return _enabled; }
        void setEnabled(bool enabled) { _enabled = enabled; }

        int logLevel() const { return _logLevel; }
        void setLogLevel(const int logLevel) { _logLevel = logLevel; }

    private:
        std::string _host;
        int _port;
        std::string _userName;
        std::string _userPassword;
        std::string _privateKeyFile;
        std::string _publicKeyFile;
        std::string _passphrase;
        std::string _authMethod; // "password" or "publickey"

        /**
         * Flag, indicating whether we should use
         * this SSH tunnel or not.
         */
        bool _enabled;
        int _logLevel; // this property is not persisted
    };
}
