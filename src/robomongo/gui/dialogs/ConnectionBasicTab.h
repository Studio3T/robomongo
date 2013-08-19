#pragma once

#include <QWidget>
QT_BEGIN_NAMESPACE
class QLineEdit;
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

    private:
        QLineEdit *_connectionName;
        QLineEdit *_serverAddress;
        QLineEdit *_serverPort;
        ConnectionSettings *_settings;
    };
}
