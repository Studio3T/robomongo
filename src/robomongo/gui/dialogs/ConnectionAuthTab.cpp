#include "robomongo/gui/dialogs/ConnectionAuthTab.h"

#include <QGridLayout>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/utils/GuiConstants.h"

namespace Robomongo
{
    ConnectionAuthTab::ConnectionAuthTab(ConnectionSettings *settings) :
        _settings(settings)
    {
        _databaseNameDescriptionLabel = new QLabel(
            "<nobr>The admin database is unique in MongoDB.</nobr> Users with normal access "
            "to the admin database have read and write access to <b>all "
            "databases</b>.");

        _databaseNameDescriptionLabel->setWordWrap(true);
        _databaseNameDescriptionLabel->setAlignment(Qt::AlignTop);
        _databaseNameDescriptionLabel->setContentsMargins(0, -2, 0, 0);
        _databaseNameDescriptionLabel->setMinimumSize(_databaseNameDescriptionLabel->sizeHint());

        _userName = new QLineEdit();
        _userNameLabel = new QLabel("User Name");
        _mechanismLabel = new QLabel("Auth Mechanism");
        _userPassword = new QLineEdit();
        _userPassword->setEchoMode(QLineEdit::Password);
        _userPasswordLabel = new QLabel("Password");
        _databaseName = new QLineEdit("admin");
        _databaseNameLabel = new QLabel("Database");

        _mechanismComboBox = new QComboBox();
        _mechanismComboBox->addItem("SCRAM-SHA-1");
        _mechanismComboBox->addItem("MONGODB-CR");

        _useAuth = new QCheckBox("Perform authentication");
        _useAuth->setStyleSheet("margin-bottom: 7px");
        VERIFY(connect(_useAuth, SIGNAL(toggled(bool)), this, SLOT(authChecked(bool))));

        _echoModeButton = new QPushButton;
        _echoModeButton->setIcon(GuiRegistry::instance().hideIcon());
#ifdef Q_OS_MAC
        _echoModeButton->setMaximumWidth(50);
#else
        _echoModeButton->setMinimumWidth(50);
#endif
        // Attempt to fix the issue for Windows High DPI button height is slightly taller than other widgets 
#ifdef Q_OS_WIN
        _echoModeButton->setMaximumHeight(HighDpiConstants::WIN_HIGH_DPI_BUTTON_HEIGHT);
#endif
        VERIFY(connect(_echoModeButton, SIGNAL(clicked()), this, SLOT(toggleEchoMode())));

        _useAuth->setChecked(_settings->hasEnabledPrimaryCredential());
        authChecked(_settings->hasEnabledPrimaryCredential());

        if (_settings->credentialCount() > 0) {
            CredentialSettings *primaryCredential = _settings->primaryCredential();
            _userName->setText(QtUtils::toQString(primaryCredential->userName()));
            _userPassword->setText(QtUtils::toQString(primaryCredential->userPassword()));
            _databaseName->setText(QtUtils::toQString(primaryCredential->databaseName()));
            _mechanismComboBox->setCurrentText(QtUtils::toQString(primaryCredential->mechanism()));
        }

        QGridLayout *authLayout = new QGridLayout;
        authLayout->addWidget(_useAuth,                      0, 0, 1, 3);
        authLayout->addWidget(_databaseNameLabel,            1, 0);
        authLayout->addWidget(_databaseName,                 1, 1, 1, 2);
        authLayout->addWidget(_databaseNameDescriptionLabel, 2, 1, 1, 2);
        authLayout->addWidget(_userNameLabel,                3, 0);
        authLayout->addWidget(_userName,                     3, 1, 1, 2);
        authLayout->addWidget(_userPasswordLabel,            4, 0);
        authLayout->addWidget(_userPassword,                 4, 1);
        authLayout->addWidget(_echoModeButton,               4, 2);
        authLayout->addWidget(_mechanismLabel,               5, 0);
        authLayout->addWidget(_mechanismComboBox,            5, 1, 1, 2);
        authLayout->setAlignment(Qt::AlignTop);
        setLayout(authLayout);
    }

    void ConnectionAuthTab::accept()
    {
        _settings->clearCredentials();

        // If all fields is empty - do nothing
        if (_userName->text().isEmpty()     &&
            _userPassword->text().isEmpty() &&
            _databaseName->text().isEmpty())
            return;       

        CredentialSettings *credential = new CredentialSettings();
        credential->setEnabled(_useAuth->isChecked());
        credential->setUserName(QtUtils::toStdString(_userName->text()));
        credential->setUserPassword(QtUtils::toStdString(_userPassword->text()));
        credential->setDatabaseName(QtUtils::toStdString(_databaseName->text()));
        credential->setMechanism(QtUtils::toStdString(_mechanismComboBox->currentText()));
        _settings->addCredential(credential);
    }

    void ConnectionAuthTab::toggleEchoMode()
    {
        bool isPassword = _userPassword->echoMode() == QLineEdit::Password;
        _userPassword->setEchoMode(isPassword ? QLineEdit::Normal: QLineEdit::Password);
        _echoModeButton->setIcon(isPassword ? GuiRegistry::instance().showIcon() : GuiRegistry::instance().hideIcon());
    }

    void ConnectionAuthTab::authChecked(bool checked)
    {
        _databaseName->setDisabled(!checked);
        _databaseNameLabel->setDisabled(!checked);
        _databaseNameDescriptionLabel->setDisabled(!checked);
        _userName->setDisabled(!checked);
        _userNameLabel->setDisabled(!checked);
        _userPassword->setDisabled(!checked);
        _userPasswordLabel->setDisabled(!checked);
        _echoModeButton->setDisabled(!checked);

        _mechanismLabel->setDisabled(!checked);
        _mechanismComboBox->setDisabled(!checked);

        if (checked)
            _databaseName->setFocus();
    }
}
