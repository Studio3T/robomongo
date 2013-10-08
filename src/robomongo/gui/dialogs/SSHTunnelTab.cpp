#include "robomongo/gui/dialogs/SSHTunnelTab.h"

#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QRegExpValidator>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>

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

        _passwordBox = new QLineEdit(QtUtils::toQString(info._password));
        
        QGridLayout *connectionLayout = new QGridLayout;
        connectionLayout->addWidget(new QLabel("SSH Host:"),          1, 0);
        connectionLayout->addWidget(_sshHostName,              1, 1);

        connectionLayout->addWidget(new QLabel("UserName:"),       2, 0);
        connectionLayout->addWidget(_userName,               2, 1);

        connectionLayout->addWidget(new QLabel("Port"),              3, 0);
        connectionLayout->addWidget(_sshPort,                  3, 1);

        connectionLayout->addWidget(new QLabel("Password"),              4, 0);
        connectionLayout->addWidget(_passwordBox,          4, 1);

        QVBoxLayout *mainLayout = new QVBoxLayout;
        mainLayout->addWidget(_sshSupport);
        mainLayout->addLayout(connectionLayout);
        setLayout(mainLayout);

        sshSupportStateChanged(_sshSupport->checkState());
        VERIFY(connect(_sshSupport,SIGNAL(stateChanged(int)),this,SLOT(sshSupportStateChanged(int))));

        _sshHostName->setFocus();
    }

    void SshTunelTab::sshSupportStateChanged(int value)
    {
        _sshHostName->setEnabled(value);
        _userName->setEnabled(value);
        _sshPort->setEnabled(value);
        _passwordBox->setEnabled(value);        
        if (!value) {
            _passwordBox->setText("");
        }
    }

    void SshTunelTab::accept()
    {
        SSHInfo info;
        info._hostName = QtUtils::toStdString(_sshHostName->text());
        info._userName = QtUtils::toStdString(_userName->text()); 
        info._port = _sshPort->text().toInt();
        info._password = QtUtils::toStdString(_passwordBox->text());

        _settings->setSshInfo(info);
    }
}
