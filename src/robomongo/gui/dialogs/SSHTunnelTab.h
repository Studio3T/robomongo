#pragma once

#include <QWidget>
QT_BEGIN_NAMESPACE
class QLineEdit;
class QCheckBox;
class QPushButton;
class QComboBox;
class QFrame;
QT_END_NAMESPACE

namespace Robomongo
{
    class ConnectionSettings;

    class SshTunelTab : public QWidget
    {
        Q_OBJECT

    public:
        SshTunelTab(ConnectionSettings *settings);
        void accept();

    private Q_SLOTS:
        void sshSupportStateChanged(int val);
        void securityChanged(const QString& val);
        void setPublicFile();
        void setPrivateFile();
        
    private:        
        QCheckBox *_sshSupport;
        QLineEdit *_sshHostName;
        QLineEdit *_userName;
        QLineEdit *_sshPort;
        QComboBox *_security;

        QFrame *_passwordFrame;
        QFrame *_pivateKeyFrame;

        QLineEdit *_passwordBox;

        QLineEdit *_publicKeyBox; 
        QLineEdit *_privateKeyBox; 
        QLineEdit *_passphraseBox;

        ConnectionSettings *const _settings;
    };
}
