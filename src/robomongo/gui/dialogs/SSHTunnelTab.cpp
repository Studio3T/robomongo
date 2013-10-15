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
        _sshSupport = new QCheckBox("SSH support");
        _sshSupport->setChecked(info.isValid());

        _sshHostName = new QLineEdit(QtUtils::toQString(info._hostName));
        _userName = new QLineEdit(QtUtils::toQString(info._userName));
        _sshPort = new QLineEdit(QString::number(info._port));
        _sshPort->setFixedWidth(80);
        QRegExp rx("\\d+");//(0-65554)
        _sshPort->setValidator(new QRegExpValidator(rx, this));        

        _security = new QComboBox();
        _security->addItems(QStringList() << "Password" << "PublicKey");
        VERIFY(connect(_security,SIGNAL(currentIndexChanged(const QString&)),this,SLOT(securityChange(const QString&))));

        _passwordBox = new QLineEdit(QtUtils::toQString(info._password));
        _publicKeyBox = new QLineEdit(QtUtils::toQString(info._publicKey._publicKey));
        _privateKeyBox = new QLineEdit(QtUtils::toQString(info._publicKey._privateKey));
        _passphraseBox = new QLineEdit(QtUtils::toQString(info._publicKey._passphrase));
#ifdef Q_OS_WIN
        QRegExp pathx("([a-zA-Z]:)?([\\\\/][a-zA-Z0-9_.-]+)+[\\\\/]?");
#else
        QRegExp pathx("^\\/?([\\d\\w\\.]+)(/([\\d\\w\\.]+))*\\/?$");
#endif // Q_OS_WIN
        _publicKeyBox->setValidator(new QRegExpValidator(pathx, this));
        _privateKeyBox->setValidator(new QRegExpValidator(pathx, this));

        QGridLayout *connectionLayout = new QGridLayout;
        connectionLayout->addWidget(new QLabel("SSH Host:"),          1, 0);
        connectionLayout->addWidget(_sshHostName,              1, 1);

        connectionLayout->addWidget(new QLabel("Username:"),       2, 0);
        connectionLayout->addWidget(_userName,               2, 1);

        connectionLayout->addWidget(new QLabel("Port:"),              3, 0);
        connectionLayout->addWidget(_sshPort,                  3, 1);

        connectionLayout->addWidget(new QLabel("Security:"),              4, 0);
        connectionLayout->addWidget(_security,          4, 1);
        
        _pivateKeyFrame = new QFrame;
        QVBoxLayout *pivL = new QVBoxLayout;
        pivL->setContentsMargins(0,0,0,0);
        QHBoxLayout *pivL1 = new QHBoxLayout;
        pivL1->addWidget(new QLabel("Public key:"));
        pivL1->addWidget(_publicKeyBox);
        QPushButton *selectPublicFile = new QPushButton("...");
        selectPublicFile->setFixedSize(20,20);       
        pivL1->addWidget(selectPublicFile);        
       
        QHBoxLayout *pivL2 = new QHBoxLayout;
        pivL2->addWidget(new QLabel("Private key:"));
        pivL2->addWidget(_privateKeyBox);
        QPushButton *selectPrivateFile = new QPushButton("...");
        selectPrivateFile->setFixedSize(20,20);       
        pivL2->addWidget(selectPrivateFile);
        
        QHBoxLayout *pivL3 = new QHBoxLayout;
        pivL3->addWidget(new QLabel("Passphrase:"));
        pivL3->addWidget(_passphraseBox);

        pivL->addLayout(pivL1);
        pivL->addLayout(pivL2);
        pivL->addLayout(pivL3);
        _pivateKeyFrame->setLayout(pivL);

        _passwordFrame = new QFrame;
        QHBoxLayout *pasL = new QHBoxLayout;
        pasL->setContentsMargins(0,0,0,0);
        pasL->addWidget(new QLabel("Password:"));
        pasL->addWidget(_passwordBox);
        _passwordFrame->setLayout(pasL);        

        QVBoxLayout *mainLayout = new QVBoxLayout;
        mainLayout->addWidget(_sshSupport);
        mainLayout->addLayout(connectionLayout);
        mainLayout->addWidget(_passwordFrame);
        mainLayout->addWidget(_pivateKeyFrame);
        setLayout(mainLayout);

        if(info.authMethod()==SSHInfo::PUBLICKEY){         
            _security->setCurrentText("PublicKey");
        }
        else{           
            _security->setCurrentText("Password");
        }

        securityChange(_security->currentText());
        VERIFY(connect(selectPrivateFile, SIGNAL(clicked()), this, SLOT(setPrivateFile())));
        VERIFY(connect(selectPublicFile, SIGNAL(clicked()), this, SLOT(setPublicFile())));

        sshSupportStateChange(_sshSupport->checkState());
        VERIFY(connect(_sshSupport,SIGNAL(stateChanged(int)),this,SLOT(sshSupportStateChange(int))));

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
        _pivateKeyFrame->setEnabled(value);
        _passwordFrame->setEnabled(value);   
    }

    void SshTunelTab::securityChange(const QString& val)
    {
        bool isPrivate = val == "PublicKey";        

        _pivateKeyFrame->setVisible(isPrivate);
        _passwordFrame->setVisible(!isPrivate);
    }

    void SshTunelTab::setPublicFile()
    {
        QString filepath = QFileDialog::getOpenFileName(this,"Select public key file",_publicKeyBox->text(),QObject::tr("Public key files (*.*)"));
        _publicKeyBox->setText(filepath);
    }

    void SshTunelTab::setPrivateFile()
    {
        QString filepath = QFileDialog::getOpenFileName(this,"Select private key file",_privateKeyBox->text(),QObject::tr("Private key files (*.*)"));
        _privateKeyBox->setText(filepath);
    }

    void SshTunelTab::accept()
    {
        SSHInfo info;
        info._hostName = QtUtils::toStdString(_sshHostName->text());
        info._userName = QtUtils::toStdString(_userName->text()); 
        info._port = _sshPort->text().toInt();
        if(_passwordFrame->isEnabled()){
            info._password = QtUtils::toStdString(_passwordBox->text());
        }
        if(_passphraseBox->isEnabled()){
            info._publicKey._publicKey = QtUtils::toStdString(_publicKeyBox->text());
            info._publicKey._privateKey = QtUtils::toStdString(_privateKeyBox->text());
            info._publicKey._passphrase = QtUtils::toStdString(_passphraseBox->text());
        }

        if (_passwordFrame->isVisible()){
            info._currentMethod = SSHInfo::PASSWORD;
        }
        else if(_passphraseBox->isVisible()){
            info._currentMethod = SSHInfo::PUBLICKEY;
        }
        
        _settings->setSshInfo(info);
    }
}
