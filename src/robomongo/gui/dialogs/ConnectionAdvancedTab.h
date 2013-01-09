#pragma once

#include <QWidget>
#include <QLineEdit>

namespace Robomongo
{
    class ConnectionSettings;

    class ConnectionAdvancedTab : public QWidget
    {
        Q_OBJECT
    public:
        ConnectionAdvancedTab(ConnectionSettings *settings);
        QLineEdit *_defaultDatabaseName;
        void accept();

    private:
        ConnectionSettings *_settings;
    };
}
