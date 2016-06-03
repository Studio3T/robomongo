#include "robomongo/gui/dialogs/SSLTab.h"

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QRegExpValidator>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QComboBox>
#include <QMessageBox>
#include <QFileInfo>
#include <QFrame>
#include <QRadioButton>
#include <QSpacerItem>
#include <QHBoxLayout>

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/utils/ComboBoxUtils.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/SshSettings.h"

namespace {
    const QString askPasswordText = "Ask for password each time";
    const QString askPassphraseText = "Ask for passphrase each time";
    bool isFileExists(const QString &path) {
        QFileInfo fileInfo(path);
        return fileInfo.exists() && fileInfo.isFile();
    }
}

namespace Robomongo
{
    SSLTab::SSLTab(ConnectionSettings *settings) 
        : _settings(settings)
    {
        //SslSettings *sslSettings = settings->sslSettings();
        _useSslCheckBox = new QCheckBox("Use SSL protocol");
        _useSslCheckBox->setStyleSheet("margin-bottom: 7px");
        //_sshSupport->setChecked(sslSettings->enabled());

        // CA Section widgets
        _acceptSelfSignedButton = new QRadioButton("Accept self-signed certificates");
        _useRootCaFileButton = new QRadioButton("Use own Root CA file ( --sslCAFile )");
        _caFileLabel = new QLabel("Root CA file: ");
        _caFilePathLineEdit = new QLineEdit;
        _caFileBrowseButton = new QPushButton("...");
        _caFileBrowseButton->setMaximumWidth(50);

        // Client Cert section widgets
        _useClientCertCheckBox = new QCheckBox("Use Client Certificate ( --sslPemKeyFile )");
        _clientCertLabel = new QLabel("Client Certificate: ");
        _clientCertPathLineEdit = new QLineEdit;
        _clientCertFileBrowseButton = new QPushButton("...");
        _clientCertFileBrowseButton->setMaximumWidth(50);
        _clientCertPassLabel = new QLabel("Passphrase: ");
        _clientCertPassLineEdit = new QLineEdit;
        _clientCertPassShowButton = new QPushButton("Show");
        _useClientCertPassCheckBox = new QCheckBox("Client certificated is protected by passphrase");

        // Layouts
        QHBoxLayout* rootCaFileLayout = new QHBoxLayout;
        rootCaFileLayout->addWidget(_caFileLabel);
        rootCaFileLayout->addWidget(_caFilePathLineEdit);
        rootCaFileLayout->addWidget(_caFileBrowseButton);

        QGridLayout* clientCertLayout = new QGridLayout;
        clientCertLayout->addWidget(_clientCertLabel,               0, 0);
        clientCertLayout->addWidget(_clientCertPathLineEdit,        0, 1);
        clientCertLayout->addWidget(_clientCertFileBrowseButton,    0, 2);
        clientCertLayout->addWidget(_clientCertPassLabel,           1, 0);
        clientCertLayout->addWidget(_clientCertPassLineEdit,        1, 1);
        clientCertLayout->addWidget(_clientCertPassShowButton,      1, 2);

        QVBoxLayout *mainLayout = new QVBoxLayout;
        mainLayout->addWidget(_useSslCheckBox);
        mainLayout->addSpacerItem(new QSpacerItem(20, 30));
        mainLayout->addWidget(_acceptSelfSignedButton);
        mainLayout->addWidget(_useRootCaFileButton);
        mainLayout->addLayout(rootCaFileLayout);
        mainLayout->addSpacerItem(new QSpacerItem(20, 30));
        mainLayout->addWidget(_useClientCertCheckBox);
        mainLayout->addLayout(clientCertLayout);
        mainLayout->addWidget(_useClientCertPassCheckBox);
        setLayout(mainLayout);
    }

}