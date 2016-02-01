#pragma once

#include <QWidget>
QT_BEGIN_NAMESPACE
class QLineEdit;
class QLabel;
class QCheckBox;
class QPushButton;
class QComboBox;
QT_END_NAMESPACE

namespace Robomongo
{
    class ConnectionSettings;

    class ConnectionAuthTab : public QWidget
    {
        Q_OBJECT

    public:
        ConnectionAuthTab(ConnectionSettings *settings);
        void accept();

    private Q_SLOTS:
        void toggleEchoMode();
        void authChecked(bool checked);

    private:
        QLineEdit *_userName;
        QLabel    *_userNameLabel;
        QLineEdit *_userPassword;
        QLabel    *_userPasswordLabel;
        QLineEdit *_databaseName;
        QLabel    *_databaseNameLabel;
        QLabel    *_databaseNameDescriptionLabel;
        QCheckBox *_useAuth;
        QPushButton *_echoModeButton;

        QLabel    *_mechanismLabel;
        QComboBox *_mechanismComboBox;

        ConnectionSettings *const _settings;        
    };
}
