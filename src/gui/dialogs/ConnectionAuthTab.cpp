#include "ConnectionAuthTab.h"
#include "settings/ConnectionSettings.h"
#include <QGridLayout>

using namespace Robomongo;

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
    _userPasswordLabel = new QLabel("Password");
    _databaseName = new QLineEdit();
    _databaseNameLabel = new QLabel("Database");

    _useAuth = new QCheckBox("Perform authentication");
    _useAuth->setStyleSheet("margin-bottom: 7px");
    connect(_useAuth, SIGNAL(toggled(bool)), this, SLOT(authChecked(bool)));

    _useAuth->setChecked(_settings->hasEnabledCredential());
    authChecked(_settings->hasEnabledCredential());

    if (_settings->credentialCount() > 0) {
        CredentialSettings *firstCredential = _settings->firstCredential();
        _userName->setText(firstCredential->userName());
        _userPassword->setText(firstCredential->userPassword());
        _databaseName->setText(firstCredential->databaseName());
    }

    QGridLayout *_authLayout = new QGridLayout;

    _authLayout->addWidget(_useAuth,                      0, 0, 1, 2);
    _authLayout->addWidget(_databaseNameLabel,            1, 0);
    _authLayout->addWidget(_databaseName,                 1, 1);
    _authLayout->addWidget(_databaseNameDescriptionLabel, 2, 1);
    _authLayout->addWidget(_userNameLabel,                3, 0);
    _authLayout->addWidget(_userName,                     3, 1);
    _authLayout->addWidget(_userPasswordLabel,            4, 0);
    _authLayout->addWidget(_userPassword,                 4, 1);
    _authLayout->setAlignment(Qt::AlignTop);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(_authLayout);
    setLayout(mainLayout);
}

void ConnectionAuthTab::accept()
{
    _settings->clearCredentials();

    // If all fields is empty - do nothing
    if (_userName->text().isEmpty()     &&
        _userPassword->text().isEmpty() &&
        _databaseName->text().isEmpty())
        return;

    _settings->addCredential(new CredentialSettings());

    CredentialSettings *credential = _settings->firstCredential();
    credential->setEnabled(_useAuth->isChecked());
    credential->setUserName(_userName->text());
    credential->setUserPassword(_userPassword->text());
    credential->setDatabaseName(_databaseName->text());
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

    if (checked)
        _databaseName->setFocus();
}
