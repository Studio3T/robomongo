#pragma once

#include <QWidget>
QT_BEGIN_NAMESPACE
class QLineEdit;
class QCheckBox;
class QPushButton;
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

    private:
        QCheckBox *_sshSupport;
        QLineEdit *_sshHostName;
        QLineEdit *_userName;
        QLineEdit *_sshPort;
        QLineEdit *_passwordBox; 

        ConnectionSettings *const _settings;
    };
}
