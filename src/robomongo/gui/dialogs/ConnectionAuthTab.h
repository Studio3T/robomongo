#pragma once

#include <QWidget>

#include "robomongo/gui/utils/GuiConstants.h"

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
        void setAuthTab(
            QString const db, 
            QString const username, 
            QString const pwd, 
            AuthMechanism authMech
        );
        void clearTab();

    private Q_SLOTS:
        void toggleEchoMode();
        void authChecked(bool checked);
        void useManuallyVisibleDbsChecked(bool checked);

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
        QCheckBox *_useManuallyVisibleDbs;
        QLineEdit *_manuallyVisibleDbs;
        QLabel    *_manuallyVisibleDbsLabel;
        QLabel    *_manuallyVisibleDbsInfo;

        ConnectionSettings *const _settings;        
    };
}
