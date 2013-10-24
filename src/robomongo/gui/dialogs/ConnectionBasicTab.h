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

    class ConnectionBasicTab : public QWidget
    {
        Q_OBJECT

    public:
        ConnectionBasicTab(ConnectionSettings *settings);
        void accept();
        bool isSslSupported() const;

    private Q_SLOTS:
        void setSslPEMKeyFile();
        void sslSupportStateChange(int val);

    private:
        QLineEdit *_connectionName;
        QLineEdit *_serverAddress;
        QLineEdit *_serverPort;
        QCheckBox *_sslSupport;
        QPushButton *_selectFileB;
        QLineEdit *_sslPEMKeyFile; 
        ConnectionSettings *const _settings;
    };
}
