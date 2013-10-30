#include "robomongo/gui/dialogs/SSHTunnelTab.h"

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
#include "robomongo/core/settings/ConnectionSettings.h"

namespace Robomongo
{
    SshTunelTab::SshTunelTab(ConnectionSettings *settings) :
        _settings(settings)
    {
        SSHInfo info = _settings->sshInfo();
        _sshSupport = new QCheckBox("Use SSH tunnel");
        _sshSupport->setStyleSheet("margin-bottom: 7px");
        _sshSupport->setChecked(info.isValid());

        _sshHostName = new QLineEdit(QtUtils::toQString(info._hostName));
        _userName = new QLineEdit(QtUtils::toQString(info._userName));

        _sshPort = new QLineEdit(QString::number(info._port));
        _sshPort->setFixedWidth(80);
        QRegExp rx("\\d+");//(0-65554)
        _sshPort->setValidator(new QRegExpValidator(rx, this));        

        _security = new QComboBox();
        _security->addItems(QStringList() << "Password" << "PublicKey");
        VERIFY(connect(_security, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(securityChange(const QString&))));

        _passwordBox = new QLineEdit(QtUtils::toQString(info._password));
        _publicKeyBox = new QLineEdit(QtUtils::toQString(info._publicKey._publicKey));
        _privateKeyBox = new QLineEdit(QtUtils::toQString(info._publicKey._privateKey));
        _passphraseBox = new QLineEdit(QtUtils::toQString(info._publicKey._passphrase));

        _passwordLabel = new QLabel("User Password:");
        _sshPublicKeyLabel = new QLabel("Public key:");
        _sshPrivateKeyLabel = new QLabel("Private key:");
        _sshPassphraseLabel = new QLabel("Passphrase:");

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

        connectionLayout->addWidget(new QLabel("SSH Host:"),       1, 0);
        connectionLayout->addLayout(hostAndPasswordLayout,         1, 1, 1, 2);

        connectionLayout->addWidget(new QLabel("User Name:"),      2, 0);
        connectionLayout->addWidget(_userName,                     2, 1, 1, 2);

        connectionLayout->addWidget(new QLabel("Auth Method:"),    4, 0);
        connectionLayout->addWidget(_security,                     4, 1, 1, 2);

        connectionLayout->addWidget(_passwordLabel,                5, 0);
        connectionLayout->addWidget(_passwordBox,                  5, 1, 1, 2);

        _selectPublicFileButton = new QPushButton("...");
        _selectPublicFileButton->setFixedSize(20, 20);

        _selectPrivateFileButton = new QPushButton("...");
        _selectPrivateFileButton->setFixedSize(20, 20);

        connectionLayout->addWidget(_sshPublicKeyLabel,            6, 0);
        connectionLayout->addWidget(_publicKeyBox,                 6, 1);
        connectionLayout->addWidget(_selectPublicFileButton,       6, 2);

        connectionLayout->addWidget(_sshPrivateKeyLabel,           7, 0);
        connectionLayout->addWidget(_privateKeyBox,                7, 1);
        connectionLayout->addWidget(_selectPrivateFileButton,      7, 2);

        connectionLayout->addWidget(_sshPassphraseLabel,           8, 0);
        connectionLayout->addWidget(_passphraseBox,                8, 1, 1, 2);

        QVBoxLayout *mainLayout = new QVBoxLayout;
        mainLayout->addWidget(_sshSupport);
        mainLayout->addLayout(connectionLayout);
        setLayout(mainLayout);

        if (info.authMethod() == SSHInfo::PUBLICKEY) {
            _security->setCurrentText("PublicKey");
        } else {
            _security->setCurrentText("Password");
        }

        securityChange(_security->currentText());
        VERIFY(connect(_selectPrivateFileButton, SIGNAL(clicked()), this, SLOT(setPrivateFile())));
        VERIFY(connect(_selectPublicFileButton, SIGNAL(clicked()), this, SLOT(setPublicFile())));

        sshSupportStateChange(_sshSupport->checkState());
        VERIFY(connect(_sshSupport, SIGNAL(stateChanged(int)), this, SLOT(sshSupportStateChange(int))));

        _sshHostName->setFocus();
    }

    bool SshTunelTab::isSshSupported() const
    {
        return _sshSupport->isChecked();
    }

    void SshTunelTab::sshSupportStateChange(int value)
    {
        _sshHostName->setEnabled(value);
        _userName->setEnabled(value);
        _sshPort->setEnabled(value);       
        _security->setEnabled(value);

        _sshPublicKeyLabel->setEnabled(value);
        _publicKeyBox->setEnabled(value);
        _selectPublicFileButton->setEnabled(value);

        _sshPrivateKeyLabel->setEnabled(value);
        _privateKeyBox->setEnabled(value);
        _selectPrivateFileButton->setEnabled(value);

        _sshPassphraseLabel->setEnabled(value);
        _passphraseBox->setEnabled(value);

        _passwordBox->setEnabled(value);
        _passwordLabel->setEnabled(value);
    }

    void SshTunelTab::securityChange(const QString&)
    {
        bool isKey = selectedAuthMethod() == SSHInfo::PUBLICKEY;

        _sshPublicKeyLabel->setVisible(isKey);
        _publicKeyBox->setVisible(isKey);
        _selectPublicFileButton->setVisible(isKey);

        _sshPrivateKeyLabel->setVisible(isKey);
        _privateKeyBox->setVisible(isKey);
        _selectPrivateFileButton->setVisible(isKey);

        _sshPassphraseLabel->setVisible(isKey);
        _passphraseBox->setVisible(isKey);

        _passwordBox->setVisible(!isKey);
        _passwordLabel->setVisible(!isKey);
    }

    void SshTunelTab::setPublicFile()
    {
        QString filepath = QFileDialog::getOpenFileName(this, "Select public key file",
            _publicKeyBox->text(), QObject::tr("Public key files (*.*)"));

        _publicKeyBox->setText(filepath);
    }

    void SshTunelTab::setPrivateFile()
    {
        QString filepath = QFileDialog::getOpenFileName(this, "Select private key file",
            _privateKeyBox->text(), QObject::tr("Private key files (*.*)"));

        _privateKeyBox->setText(filepath);
    }

    SSHInfo::SupportedAuthenticationMetods SshTunelTab::selectedAuthMethod()
    {
        if (_security->currentText() == "PublicKey")
            return SSHInfo::PUBLICKEY;

        return SSHInfo::PASSWORD;
    }

    void SshTunelTab::accept()
    {
        SSHInfo info = _settings->sshInfo();
        info._hostName = QtUtils::toStdString(_sshHostName->text());
        info._userName = QtUtils::toStdString(_userName->text()); 
        info._port = _sshPort->text().toInt();
        info._password = QtUtils::toStdString(_passwordBox->text());
        info._publicKey._publicKey = QtUtils::toStdString(_publicKeyBox->text());
        info._publicKey._privateKey = QtUtils::toStdString(_privateKeyBox->text());
        info._publicKey._passphrase = QtUtils::toStdString(_passphraseBox->text());

        if (_sshSupport->isChecked()) {
            info._currentMethod = selectedAuthMethod();
        } else {
            info._currentMethod = SSHInfo::UNKNOWN;
        }
        
        _settings->setSshInfo(info);
    }
}
