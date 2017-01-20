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

    class ConnectionAdvancedTab : public QWidget
    {
        Q_OBJECT

    public:
        ConnectionAdvancedTab(ConnectionSettings *settings);
        void accept();

    /* --- Disabling unfinished export URI connection string feature
    private Q_SLOTS :
        void on_generateButton_clicked();
        void on_copyButton_clicked();
        void on_includePasswordsCheckBox_toggle(bool checked);
    */

    private:
        QLineEdit *_defaultDatabaseName;

        /* --- Disabling unfinished export URI connection string feature
        QLineEdit *_uriString;
        QCheckBox *_includePasswordCheckBox;
        QPushButton *_copyButton;
        */

        ConnectionSettings *_settings;
    };
}
