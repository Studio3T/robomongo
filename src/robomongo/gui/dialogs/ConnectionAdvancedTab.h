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
        void accept();

    private Q_SLOTS :
        void on_copyButton_clicked();

    private:
        QLineEdit *_defaultDatabaseName;
        QLineEdit *_uriString;

        ConnectionSettings *_settings;
    };
}
