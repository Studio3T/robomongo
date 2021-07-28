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

namespace Robomongo
{
    ConnectionAuthTab::ConnectionAuthTab(ConnectionSettings *settings) :
        _settings(settings)
    {
        _useAuth = new QCheckBox("Perform authentication");
        _useAuth->setStyleSheet("margin-bottom: 7px");
        VERIFY(connect(_useAuth, SIGNAL(toggled(bool)), this, SLOT(authChecked(bool))));

        _databaseNameDescriptionLabel = new QLabel(
            "<nobr>The admin database is unique in MongoDB.</nobr> Users with normal access "
            "to the admin database have read and write access to <b>all "
            "databases</b>."
        );
        _databaseNameDescriptionLabel->setWordWrap(true);

        _userName = new QLineEdit();
        _userNameLabel = new QLabel("User Name");
        _mechanismLabel = new QLabel("Auth Mechanism");
        _userPassword = new QLineEdit();
        _userPassword->setEchoMode(QLineEdit::Password);
        _userPasswordLabel = new QLabel("Password");
        _databaseName = new QLineEdit("admin");
        _databaseNameLabel = new QLabel("Database");

        _mechanismComboBox = new QComboBox;
        _mechanismComboBox->addItem("SCRAM-SHA-1");
        _mechanismComboBox->addItem("SCRAM-SHA-256");
        _mechanismComboBox->addItem("MONGODB-CR");

        _manuallyVisibleDbs = new QLineEdit;
        _manuallyVisibleDbs->setPlaceholderText("Comma-separated e.g. products, users");
        _manuallyVisibleDbsLabel = new QLabel("Databases");
        _manuallyVisibleDbsInfo = new QLabel(
            "Some MongoDB users might not have the permission to get the list of"
            " database names (<b>listDatabases</b> command). For this case, manually add"
            " the name of the database(s) that this user has access to."
        );
        _manuallyVisibleDbsInfo->setWordWrap(true);

        _useManuallyVisibleDbs = new QCheckBox("Manually specify visible databases");
        _useManuallyVisibleDbs->setStyleSheet("margin-bottom: 7px");
        VERIFY(connect(_useManuallyVisibleDbs, SIGNAL(toggled(bool)), 
                       this, SLOT(useManuallyVisibleDbsChecked(bool))));

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
            _useManuallyVisibleDbs->setChecked(_settings->primaryCredential()->useManuallyVisibleDbs());
            _manuallyVisibleDbs->setText(QtUtils::toQString(primaryCredential->manuallyVisibleDbs()));
        }
        useManuallyVisibleDbsChecked(_useManuallyVisibleDbs->isChecked());

        auto horline = new QFrame;
        horline->setFrameShape(QFrame::HLine);
        horline->setFrameShadow(QFrame::Sunken);

        auto verSpacer { new QSpacerItem(0, 80, QSizePolicy::Minimum, QSizePolicy::Expanding) };

        auto authLayout = new QGridLayout;
        authLayout->addWidget(_useAuth,                      0, 0, 1, 3);
        authLayout->addWidget(_databaseNameLabel,            1, 0);
        authLayout->addWidget(_databaseName,                 1, 1, 1, 2);
        authLayout->addWidget(_databaseNameDescriptionLabel, 2, 1, 1, 2);
        authLayout->addWidget(new QLabel,                    3, 0);
        authLayout->addWidget(_userNameLabel,                4, 0);
        authLayout->addWidget(_userName,                     4, 1, 1, 2);
        authLayout->addWidget(_userPasswordLabel,            5, 0);
        authLayout->addWidget(_userPassword,                 5, 1);
        authLayout->addWidget(_echoModeButton,               5, 2);
        authLayout->addWidget(_mechanismLabel,               6, 0);
        authLayout->addWidget(_mechanismComboBox,            6, 1, 1, 2);
        authLayout->addWidget(new QLabel,                    7, 0);
        authLayout->addWidget(horline,                       8, 0, 1, 3);
        authLayout->addWidget(_useManuallyVisibleDbs,        9, 0, 1, 3);
        authLayout->addWidget(_manuallyVisibleDbsLabel,     10, 0);
        authLayout->addWidget(_manuallyVisibleDbs,          10, 1, 1, 2);
        authLayout->addWidget(_manuallyVisibleDbsInfo,      11, 1, 1, 2);        
        authLayout->addItem(verSpacer,                      12, 0, 1, 3); 
        
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
        
        auto credential { new CredentialSettings };
        credential->setEnabled(_useAuth->isChecked());
        credential->setUserName(QtUtils::toStdString(_userName->text()));
        credential->setUserPassword(QtUtils::toStdString(_userPassword->text()));
        credential->setDatabaseName(QtUtils::toStdString(_databaseName->text()));
        credential->setMechanism(QtUtils::toStdString(_mechanismComboBox->currentText()));
        credential->setUseManuallyVisibleDbs(
            _useManuallyVisibleDbs->isChecked() && !_manuallyVisibleDbs->text().isEmpty()
        );
        credential->setManuallyVisibleDbs(
            _manuallyVisibleDbs->text().simplified().replace(' ', "").toStdString()
        );
        _settings->addCredential(credential);
    }

    void ConnectionAuthTab::setAuthTab(
        QString const db, 
        QString const username, 
        QString const pwd, 
        AuthMechanism authMech
    ) {    
        _useAuth->setChecked(true);
        _databaseName->setText(db);
        _userName->setText(username);
        _userPassword->setText(pwd);
        _mechanismComboBox->setCurrentIndex(int(authMech));
    }

    void ConnectionAuthTab::clearTab()
    {
        _useAuth->setChecked(false);
        _databaseName->clear();
        _userName->clear();
        _userPassword->clear();
        _mechanismComboBox->setCurrentIndex(0);
        _useManuallyVisibleDbs->setChecked(false);
        _manuallyVisibleDbs->clear();        
    }

    void ConnectionAuthTab::toggleEchoMode()
    {
        bool isPassword = _userPassword->echoMode() == QLineEdit::Password;
        _userPassword->setEchoMode(isPassword ? QLineEdit::Normal: QLineEdit::Password);
        _echoModeButton->setIcon(isPassword ? GuiRegistry::instance().showIcon() : GuiRegistry::instance().hideIcon());
    }

    void ConnectionAuthTab::authChecked(bool checked)
    {
        _databaseName->setEnabled(checked);
        _databaseNameLabel->setEnabled(checked);
        _databaseNameDescriptionLabel->setEnabled(checked);
        _userName->setEnabled(checked);
        _userNameLabel->setEnabled(checked);
        _userPassword->setEnabled(checked);
        _userPasswordLabel->setEnabled(checked);
        _echoModeButton->setEnabled(checked);
        _mechanismLabel->setEnabled(checked);
        _mechanismComboBox->setEnabled(checked);
        _useManuallyVisibleDbs->setEnabled(checked);
        _manuallyVisibleDbs->setEnabled(checked);
        _manuallyVisibleDbsLabel->setEnabled(checked);
        _manuallyVisibleDbsInfo->setEnabled(checked);

        if (checked)
            _databaseName->setFocus();
    }
    void ConnectionAuthTab::useManuallyVisibleDbsChecked(bool checked)
    {
        _manuallyVisibleDbs->setVisible(checked);
        _manuallyVisibleDbsLabel->setVisible(checked);
        _manuallyVisibleDbsInfo->setVisible(checked);
    }
}
