#pragma once

#include <QWidget>
#include <QLineEdit>

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
