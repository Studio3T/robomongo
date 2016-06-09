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
#include "robomongo/core/settings/SslSettings.h"

#include "mongo/util/net/ssl_options.h"
#include "mongo/util/net/ssl_manager.h"
//#include "mongo/client/init.h"

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
        SslSettings *sslSettings = settings->sslSettings();
        _useSslCheckBox = new QCheckBox("Use SSL protocol");
        _useSslCheckBox->setStyleSheet("margin-bottom: 7px");
        _useSslCheckBox->setChecked(sslSettings->enabled());
        VERIFY(connect(_useSslCheckBox, SIGNAL(stateChanged(int)), this, SLOT(useSslCheckBoxStateChange(int))));

        // CA Section widgets
        _acceptSelfSignedButton = new QRadioButton("Accept self-signed certificates");
        _useRootCaFileButton = new QRadioButton("Use own Root CA file ( --sslCAFile )");
        _caFileLabel = new QLabel("Root CA file: ");
        _caFilePathLineEdit = new QLineEdit;
        _caFileBrowseButton = new QPushButton("...");
        _caFileBrowseButton->setMaximumWidth(50);
        VERIFY(connect(_caFileBrowseButton, SIGNAL(clicked()), this, SLOT(on_caFileBrowseButton_clicked())));


        // Client Cert section widgets
        _useClientCertCheckBox = new QCheckBox("Use Client Certificate ( --sslPemKeyFile )");
        _clientCertLabel = new QLabel("Client Certificate: ");
        _clientCertPathLineEdit = new QLineEdit;
        _clientCertFileBrowseButton = new QPushButton("...");
        _clientCertFileBrowseButton->setMaximumWidth(50);
        VERIFY(connect(_clientCertFileBrowseButton, SIGNAL(clicked()), this, SLOT(on_pemKeyFileBrowseButton_clicked())));

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

        // Update widgets according to settings
        useSslCheckBoxStateChange(_useSslCheckBox->checkState());
        _caFilePathLineEdit->setText(QString::fromStdString(sslSettings->caFile()));
        _clientCertPathLineEdit->setText(QString::fromStdString(sslSettings->pemKeyFile()));
    }

    void SSLTab::on_caFileBrowseButton_clicked()
    {
        // If user has previously selected a file, initialize file dialog
        // with that file's name; otherwise, use user's home directory.
        QString initialName = _caFilePathLineEdit->text();
        if (initialName.isEmpty())
        {
            initialName = QDir::homePath();
        }
        QString fileName =  QFileDialog::getOpenFileName(this, tr("Choose File"));
        fileName = QDir::toNativeSeparators(fileName);
        if (!fileName.isEmpty()) {
            _caFilePathLineEdit->setText(fileName);
            //buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        }

        //mongo::sslGlobalParams.sslMode.store(mongo::SSLParams::SSLMode_requireSSL);
        //mongo::sslGlobalParams.sslPEMKeyFile = "C:\\cygwin64\\etc\\ssl\\mongodb.pem";

        //mongo::DBClientConnection *conn = new mongo::DBClientConnection(true, 10);
        ////mongo::Status status = conn->connect(mongo::HostAndPort("46.101.137.132", 27020));
        //mongo::Status status = conn->connect(mongo::HostAndPort("localhost", 27017));

        //mongo::sslGlobalParams.sslMode.store(mongo::SSLParams::SSLMode_allowSSL);

        //_clientCertPassLineEdit->setText("isOk: " + QString::number(status.isOK()));
    }

    void SSLTab::on_pemKeyFileBrowseButton_clicked()
    {
        // If user has previously selected a file, initialize file dialog
        // with that file's name; otherwise, use user's home directory.
        QString initialName = _clientCertPathLineEdit->text();
        if (initialName.isEmpty())
        {
            initialName = QDir::homePath();
        }
        QString fileName = QFileDialog::getOpenFileName(this, tr("Choose File"));
        fileName = QDir::toNativeSeparators(fileName);
        if (!fileName.isEmpty()) {
            _clientCertPathLineEdit->setText(fileName);
            //buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        }
    }

    void SSLTab::useSslCheckBoxStateChange(int state)
    {
        bool isChecked = static_cast<bool>(state);
        _acceptSelfSignedButton->setDisabled(!isChecked);
        _useRootCaFileButton->setDisabled(!isChecked);
        _caFileLabel->setDisabled(!isChecked);
        _caFilePathLineEdit->setDisabled(!isChecked);
        _caFileBrowseButton->setDisabled(!isChecked);
        _clientCertLabel->setDisabled(!isChecked);
        _clientCertPathLineEdit->setDisabled(!isChecked);
        _clientCertFileBrowseButton->setDisabled(!isChecked);
        _clientCertPassLabel->setDisabled(!isChecked);
        _clientCertPassLineEdit->setDisabled(!isChecked);
        _clientCertPassShowButton->setDisabled(!isChecked);
        _useClientCertCheckBox->setDisabled(!isChecked);
        _useClientCertPassCheckBox->setDisabled(!isChecked);
    }

    bool SSLTab::accept()
    {
        SslSettings *sslSettings = _settings->sslSettings();
        sslSettings->setCaFile(QtUtils::toStdString(_caFilePathLineEdit->text()));
        sslSettings->setPemKeyFile(QtUtils::toStdString(_clientCertPathLineEdit->text()));
        sslSettings->enableSSL(_useSslCheckBox->isChecked());
        return true;
    }
}

