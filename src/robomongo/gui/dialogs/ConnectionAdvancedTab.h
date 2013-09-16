#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

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
