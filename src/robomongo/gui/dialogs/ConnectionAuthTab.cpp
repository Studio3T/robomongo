#include "robomongo/gui/dialogs/ConnectionAuthTab.h"

#include <QGridLayout>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    ConnectionAuthTab::ConnectionAuthTab(ConnectionSettings *settings) :
        _settings(settings)
    {
        _databaseNameDescriptionLabel = new QLabel(
            "<nobr>The <code>admin</code> database is unique in MongoDB.</nobr> Users with normal access "
            "to the <code>admin</code> database have read and write access to <b>all "
            "databases</b>.");

        _databaseNameDescriptionLabel->setWordWrap(true);
        _databaseNameDescriptionLabel->setAlignment(Qt::AlignTop);
        _databaseNameDescriptionLabel->setContentsMargins(0, -2, 0, 0);
        _databaseNameDescriptionLabel->setMinimumSize(_databaseNameDescriptionLabel->sizeHint());

        _userName = new QLineEdit();
        _userNameLabel = new QLabel("User Name");
        _userPassword = new QLineEdit();
        _userPassword->setEchoMode(QLineEdit::Password);
        _userPasswordLabel = new QLabel("Password");
        _databaseName = new QLineEdit("admin");
        _databaseNameLabel = new QLabel("Database");

        _useAuth = new QCheckBox("Perform authentication");
        _useAuth->setStyleSheet("margin-bottom: 7px");
        VERIFY(connect(_useAuth, SIGNAL(toggled(bool)), this, SLOT(authChecked(bool))));

        _echoModeButton = new QPushButton("Show");
        VERIFY(connect(_echoModeButton, SIGNAL(clicked()), this, SLOT(toggleEchoMode())));
        CredentialSettings primCred = _settings->primaryCredential();
        _useAuth->setChecked(primCred.isValidAnEnabled());
        authChecked(primCred.isValidAnEnabled());

        if (primCred.info().isValid()) {
            CredentialSettings::CredentialInfo info = primCred.info();
            _userName->setText(QtUtils::toQString(info._userName));
            _userPassword->setText(QtUtils::toQString(info._userPassword));
            _databaseName->setText(QtUtils::toQString(info._databaseName));
        }

        QGridLayout *_authLayout = new QGridLayout;
        _authLayout->addWidget(_useAuth,                      0, 0, 1, 3);
        _authLayout->addWidget(_databaseNameLabel,            1, 0);
        _authLayout->addWidget(_databaseName,                 1, 1, 1, 2);
        _authLayout->addWidget(_databaseNameDescriptionLabel, 2, 1, 1, 2);
        _authLayout->addWidget(_userNameLabel,                3, 0);
        _authLayout->addWidget(_userName,                     3, 1, 1, 2);
        _authLayout->addWidget(_userPasswordLabel,            4, 0);
        _authLayout->addWidget(_userPassword,                 4, 1);
        _authLayout->addWidget(_echoModeButton,               4, 2);
        _authLayout->setAlignment(Qt::AlignTop);

        QVBoxLayout *mainLayout = new QVBoxLayout;
        mainLayout->addLayout(_authLayout);
        setLayout(mainLayout);
    }

    void ConnectionAuthTab::accept()
    {

        CredentialSettings result;
        CredentialSettings::CredentialInfo info(QtUtils::toStdString(_userName->text()),QtUtils::toStdString(_userPassword->text()),QtUtils::toStdString(_databaseName->text()));

        if (info.isValid()){      
            result.setEnabled(_useAuth->isChecked());        
            result.setInfo(info);
        }       

        _settings->setPrimaryCredential(result);
    }

    void ConnectionAuthTab::toggleEchoMode()
    {
        bool isPassword = _userPassword->echoMode() == QLineEdit::Password;
        _userPassword->setEchoMode(isPassword ? QLineEdit::Normal: QLineEdit::Password);
        _echoModeButton->setText(isPassword ? "Hide": "Show");
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

        if (checked)
            _databaseName->setFocus();
    }
}
