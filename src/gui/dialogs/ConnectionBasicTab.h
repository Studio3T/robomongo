#ifndef CONNECTIONBASICTABWIDGET_H
#define CONNECTIONBASICTABWIDGET_H

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
        QLineEdit *_defaultDatabaseName;
        ConnectionSettings *_settings;
    };
}

#endif // CONNECTIONBASICTABWIDGET_H
