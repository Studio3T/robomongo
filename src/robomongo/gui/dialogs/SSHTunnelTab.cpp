#include "robomongo/gui/dialogs/SSHTunnelTab.h"

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QRegExpValidator>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QComboBox>
#include <QFrame>

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/utils/ComboBoxUtils.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/SshSettings.h"

namespace Robomongo
{
    SshTunnelTab::SshTunnelTab(ConnectionSettings *settings) :
        _settings(settings)
    {
        SshSettings *info = settings->sshSettings();
        _sshSupport = new QCheckBox("Use SSH tunnel");
        _sshSupport->setStyleSheet("margin-bottom: 7px");
        _sshSupport->setChecked(info->enabled());

        _sshHostName = new QLineEdit(QtUtils::toQString(info->host()));
        _userName = new QLineEdit(QtUtils::toQString(info->userName()));

        _sshPort = new QLineEdit(QString::number(info->port()));
        _sshPort->setFixedWidth(80);
        QRegExp rx("\\d+"); //(0-65554)
        _sshPort->setValidator(new QRegExpValidator(rx, this));        

        _security = new QComboBox();
        _security->addItems(QStringList() << "Password" << "Private Key");
        VERIFY(connect(_security, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(securityChange(const QString&))));

        _passwordBox = new QLineEdit(QtUtils::toQString(info->userPassword()));
        _passwordBox->setEchoMode(QLineEdit::Password);
        _passwordEchoModeButton = new QPushButton(tr("Show"));
        VERIFY(connect(_passwordEchoModeButton, SIGNAL(clicked()), this, SLOT(togglePasswordEchoMode())));
        
        _privateKeyBox = new QLineEdit(QtUtils::toQString(info->privateKeyFile()));
        
        _passphraseBox = new QLineEdit(QtUtils::toQString(info->passphrase()));
        _passphraseBox->setEchoMode(QLineEdit::Password);
        _passphraseEchoModeButton = new QPushButton(tr("Show"));
        VERIFY(connect(_passphraseEchoModeButton, SIGNAL(clicked()), this, SLOT(togglePassphraseEchoMode())));

        _passwordLabel = new QLabel("User Password:");
        _sshPrivateKeyLabel = new QLabel("Private key:");
        _sshPassphraseLabel = new QLabel("Passphrase:");
        _sshAddressLabel = new QLabel("SSH Address:");
        _sshUserNameLabel = new QLabel("SSH User Name:");
        _sshAuthMethodLabel = new QLabel("SSH Auth Method:");

/*
// Commented because of this:
// https://github.com/paralect/robomongo/issues/391

#ifdef Q_OS_WIN
        QRegExp pathx("([a-zA-Z]:)?([\\\\/][a-zA-Z0-9_.-]+)+[\\\\/]?");
#else
        QRegExp pathx("^\\/?([\\d\\w\\.]+)(/([\\d\\w\\.]+))*\\/?$");
#endif // Q_OS_WIN
        _publicKeyBox->setValidator(new QRegExpValidator(pathx, this));
        _privateKeyBox->setValidator(new QRegExpValidator(pathx, this));
*/

        QHBoxLayout *hostAndPasswordLayout = new QHBoxLayout;
        hostAndPasswordLayout->addWidget(_sshHostName);
        hostAndPasswordLayout->addWidget(new QLabel(":"));
        hostAndPasswordLayout->addWidget(_sshPort);

        QGridLayout *connectionLayout = new QGridLayout;
        connectionLayout->setAlignment(Qt::AlignTop);
        connectionLayout->setColumnStretch(1, 1);
        connectionLayout->setColumnMinimumWidth(0, _passwordLabel->sizeHint().width() + 5);

        connectionLayout->addWidget(_sshAddressLabel ,             1, 0);
        connectionLayout->addLayout(hostAndPasswordLayout,         1, 1, 1, 2);

        connectionLayout->addWidget(_sshUserNameLabel,             2, 0);
        connectionLayout->addWidget(_userName,                     2, 1, 1, 2);

        connectionLayout->addWidget(_sshAuthMethodLabel,           4, 0);
        connectionLayout->addWidget(_security,                     4, 1, 1, 2);

        connectionLayout->addWidget(_passwordLabel,                5, 0);
        connectionLayout->addWidget(_passwordBox,                  5, 1);
        connectionLayout->addWidget(_passwordEchoModeButton,       5, 2);

        _selectPrivateFileButton = new QPushButton("...");
        _selectPrivateFileButton->setMaximumWidth(50);

        connectionLayout->addWidget(_sshPrivateKeyLabel,           7, 0);
        connectionLayout->addWidget(_privateKeyBox,                7, 1);
        connectionLayout->addWidget(_selectPrivateFileButton,      7, 2);

        connectionLayout->addWidget(_sshPassphraseLabel,           8, 0);
        connectionLayout->addWidget(_passphraseBox,                8, 1);
        connectionLayout->addWidget(_passphraseEchoModeButton,     8, 2);

        QVBoxLayout *mainLayout = new QVBoxLayout;
        mainLayout->addWidget(_sshSupport);
        mainLayout->addLayout(connectionLayout);
        setLayout(mainLayout);

        if (info->authMethod() == "publickey") {
            utils::setCurrentText(_security, "Private Key");
        } else {
            utils::setCurrentText(_security, "Password");
        }

        securityChange(_security->currentText());
        VERIFY(connect(_selectPrivateFileButton, SIGNAL(clicked()), this, SLOT(setPrivateFile())));

        sshSupportStateChange(_sshSupport->checkState());
        VERIFY(connect(_sshSupport, SIGNAL(stateChanged(int)), this, SLOT(sshSupportStateChange(int))));

        _sshHostName->setFocus();
    }

    bool SshTunnelTab::isSshSupported() const
    {
        return _sshSupport->isChecked();
    }

    void SshTunnelTab::sshSupportStateChange(int checked)
    {
        _sshHostName->setEnabled(checked);
        _userName->setEnabled(checked);
        _sshPort->setEnabled(checked);
        _security->setEnabled(checked);

        _sshPrivateKeyLabel->setEnabled(checked);
        _privateKeyBox->setEnabled(checked);
        _selectPrivateFileButton->setEnabled(checked);
        _sshAddressLabel->setEnabled(checked);
        _sshUserNameLabel->setEnabled(checked);
        _sshAuthMethodLabel->setEnabled(checked);

        _sshPassphraseLabel->setEnabled(checked);
        _passphraseBox->setEnabled(checked);

        _passwordBox->setEnabled(checked);
        _passwordLabel->setEnabled(checked);

        if (checked)
            _sshHostName->setFocus();
    }

    void SshTunnelTab::securityChange(const QString& method)
    {
        bool isKey = method == "Private Key";

        _sshPrivateKeyLabel->setVisible(isKey);
        _privateKeyBox->setVisible(isKey);
        _selectPrivateFileButton->setVisible(isKey);

        _sshPassphraseLabel->setVisible(isKey);
        _passphraseBox->setVisible(isKey);
        _passphraseEchoModeButton->setVisible(isKey);
        
        _passwordBox->setVisible(!isKey);
        _passwordLabel->setVisible(!isKey);
        _passwordEchoModeButton->setVisible(!isKey);
    }

    void SshTunnelTab::setPrivateFile()
    {
        // Default location
        QString sshDir = QString("%1/.ssh").arg(QDir::homePath());

        QString filepath = QFileDialog::getOpenFileName(this, "Select private key file",
            sshDir, QObject::tr("Private key files (*)"));

        // Some strange behaviour at least on Mac happens when you
        // close QFileDialog. Focus switched to a different modal
        // dialog, not the one that was active before openning QFileDialog.
        // http://stackoverflow.com/questions/17998811/window-modal-qfiledialog-pushing-parent-to-background-after-exec
        QApplication::activeModalWidget()->raise();
        QApplication::activeModalWidget()->activateWindow();

        if (filepath.isNull())
            return;

        _privateKeyBox->setText(filepath);
    }

//    SSHInfo::SupportedAuthenticationMetods SshTunnelTab::selectedAuthMethod()
//    {
//        if (_security->currentText() == "Private Key")
//            return SSHInfo::PUBLICKEY;
//
//        return SSHInfo::PASSWORD;
//    }

    void SshTunnelTab::accept()
    {
        SshSettings *info = _settings->sshSettings();
        info->setHost(QtUtils::toStdString(_sshHostName->text()));
        info->setPort(_sshPort->text().toInt());
        info->setUserName(QtUtils::toStdString(_userName->text()));
        info->setUserPassword(QtUtils::toStdString(_passwordBox->text()));
//        info->setPublicKeyFile("");
        info->setPrivateKeyFile(QtUtils::toStdString(_privateKeyBox->text()));
        info->setPassphrase(QtUtils::toStdString(_passphraseBox->text()));

        if (_security->currentText() == "Private Key") {
            info->setAuthMethod("publickey");
        } else {
            info->setAuthMethod("password");
        }

        info->setEnabled(_sshSupport->isChecked());
    }
    
    void SshTunnelTab::togglePasswordEchoMode()
    {
        bool isPassword = _passwordBox->echoMode() == QLineEdit::Password;
        _passwordBox->setEchoMode(isPassword ? QLineEdit::Normal: QLineEdit::Password);
        _passwordEchoModeButton->setText(isPassword ? tr("Hide"): tr("Show"));
    }
    
    void SshTunnelTab::togglePassphraseEchoMode()
    {
        bool isPassword = _passphraseBox->echoMode() == QLineEdit::Password;
        _passphraseBox->setEchoMode(isPassword ? QLineEdit::Normal: QLineEdit::Password);
        _passphraseEchoModeButton->setText(isPassword ? tr("Hide"): tr("Show"));
    }
}
