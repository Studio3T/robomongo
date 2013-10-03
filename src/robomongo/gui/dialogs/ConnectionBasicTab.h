#pragma once

#include <QWidget>
QT_BEGIN_NAMESPACE
class QLineEdit;
class QCheckBox;
QT_END_NAMESPACE

namespace Robomongo
{
    class ConnectionSettings;

    class ConnectionBasicTab : public QWidget
    {
        Q_OBJECT

    public:
        ConnectionBasicTab(ConnectionSettings *settings);
        void accept();

    private Q_SLOTS:
        void setSslPEMKeyFile();

    private:
        QLineEdit *_connectionName;
        QLineEdit *_serverAddress;
        QLineEdit *_serverPort;
        QCheckBox *_sslSupport;
        QLineEdit *_sslPEMKeyFile; 
        ConnectionSettings *const _settings;
    };
}
