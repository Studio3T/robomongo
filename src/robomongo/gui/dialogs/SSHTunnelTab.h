#pragma once

#include <QWidget>

#include "robomongo/core/settings/ConnectionSettings.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QCheckBox;
class QPushButton;
class QComboBox;
class QFrame;
QT_END_NAMESPACE

namespace Robomongo
{
    class ConnectionSettings;

    class SshTunnelTab : public QWidget
    {
        Q_OBJECT

    public:
        SshTunnelTab(ConnectionSettings *settings);
        void accept();
        bool isSshSupported() const;

    private Q_SLOTS:
        void sshSupportStateChange(int val);
        void securityChange(const QString& val);
        void setPrivateFile();
        void togglePasswordEchoMode();
        void togglePassphraseEchoMode();

    private:
        SSHInfo::SupportedAuthenticationMetods selectedAuthMethod();
        
    private:        
        QCheckBox *_sshSupport;
        QLineEdit *_sshHostName;
        QLineEdit *_userName;
        QLineEdit *_sshPort;
        QComboBox *_security;
        QLabel *_sshPrivateKeyLabel;
        QLabel *_sshPassphraseLabel;
        QLabel *_sshAddressLabel;
        QLabel *_sshUserNameLabel;
        QLabel *_sshAuthMethodLabel;

        QPushButton *_selectPrivateFileButton;

        QLineEdit *_passwordBox;
        QLabel *_passwordLabel;
        QPushButton *_passwordEchoModeButton;

        QLineEdit *_privateKeyBox; 
        QLineEdit *_passphraseBox;
        QPushButton *_passphraseEchoModeButton;

        ConnectionSettings *const _settings;
    };
}
