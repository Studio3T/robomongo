#include "robomongo/gui/dialogs/SSLTab.h"

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QComboBox>
#include <QRadioButton>
#include <QFileInfo>
#include <QMessageBox>

#include <mongo/util/net/ssl_options.h>
#include <mongo/util/net/ssl_manager.h>

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/SshSettings.h"
#include "robomongo/core/settings/SslSettings.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/utils/ComboBoxUtils.h"
#include "robomongo/gui/utils/GuiConstants.h"

namespace 
{
    // Helper function: Check file existence, return true if file exists, false otherwise
    bool fileExists(const QString &path) 
    {
        QFileInfo fileInfo(path);
        return (fileInfo.exists() && fileInfo.isFile());
    }

    // Helper hint strings
    QString const CA_FILE_HINT  = " mongo --tlsCAFile : Certificate Authority file for TLS";
    QString const PEM_FILE_HINT = " mongo --tlsCertificateKeyFile : PEM certificate/key file for TLS";
    QString const PEM_PASS_HINT = " mongo --tlsCertificateKeyFilePassword : Password for key in PEM file for TLS";
    QString const ALLOW_INVALID_HOSTNAME_HINT     = " mongo --tlsAllowInvalidHostnames : Allow connections "
                                                    "to servers with non-matching hostnames";
    QString const ALLOW_INVALID_CERTIFICATES_HINT = " mongo --tlsAllowInvalidCertificates : Allow connections "
                                                    "to servers with invalid certificates";
    QString const CRL_FILE_HINT = " mongo --tlsCRLFile : Certificate Revocation List file for TLS";
}

namespace Robomongo
{
    SSLTab::SSLTab(ConnectionSettings *connSettings) 
        : _connSettings(connSettings)
    {
        const SslSettings* const sslSettings = _connSettings->sslSettings();

        // Use TLS section
        _useSslCheckBox = new QCheckBox("Use TLS protocol");
        _useSslCheckBox->setStyleSheet("margin-bottom: 7px");
        VERIFY(connect(_useSslCheckBox, SIGNAL(stateChanged(int)), this, SLOT(useSslCheckBoxStateChange(int))));

        // Auth. Method section
        _authMethodLabel = new QLabel("Authentication Method: ");
        _authMethodComboBox = new QComboBox;
        _authMethodComboBox->addItem("Self-signed Certificate");
        _authMethodComboBox->addItem("Use CA Certificate");
        _authMethodComboBox->setItemData(0, ALLOW_INVALID_CERTIFICATES_HINT, Qt::ToolTipRole);
        _authMethodComboBox->setItemData(1, CA_FILE_HINT, Qt::ToolTipRole);
        _selfSignedInfoStr = new QLabel("In general, avoid using self-signed certificates unless the network is trusted. "
            "If self-signed certificate is used, the communications channel will be encrypted however there will be "
            "no validation of server identity.");
        _selfSignedInfoStr->setWordWrap(true);
        _selfSignedInfoStr->setToolTip(ALLOW_INVALID_CERTIFICATES_HINT);
        _caFileLabel = new QLabel("CA Certificate:");
        _caFileLabel->setToolTip(CA_FILE_HINT);
        _caFilePathLineEdit = new QLineEdit;
        _caFileBrowseButton = new QPushButton("...");
        _caFileBrowseButton->setMaximumWidth(50);
        VERIFY(connect(_caFileBrowseButton, SIGNAL(clicked()), this, SLOT(on_caFileBrowseButton_clicked())));
        VERIFY(connect(_authMethodComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(on_authModeComboBox_change(int))));

        // PEM file section
        _usePemFileCheckBox = new QCheckBox("Use PEM Cert./Key: ");
        _usePemFileCheckBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        _pemFileInfoStr = 
            new QLabel("Enable this option to connect to a MongoDB that requires CA-signed client certificates/key file.");
        _pemFileInfoStr->setWordWrap(true);
#ifdef Q_OS_WIN
        _pemFileInfoStr->setContentsMargins(0,2,0,0);   // Top alignment adjustment required only for Windows
#endif
        _pemFileLabel = new QLabel("PEM Certificate/Key: ");
        _pemFileLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        _pemFileLabel->setToolTip(PEM_FILE_HINT);
        _pemFilePathLineEdit = new QLineEdit;
        _pemFileBrowseButton = new QPushButton("...");
        _pemFileBrowseButton->setMaximumWidth(50);
        VERIFY(connect(_usePemFileCheckBox, SIGNAL(toggled(bool)), this, SLOT(on_usePemFileCheckBox_toggle(bool))));
        VERIFY(connect(_pemFileBrowseButton, SIGNAL(clicked()), this, SLOT(on_pemKeyFileBrowseButton_clicked())));
        // PEM Passphrase section
        _pemPassLabel = new QLabel("Passphrase: ");    
        _pemPassLabel->setToolTip(PEM_PASS_HINT);
        _pemPassLineEdit = new QLineEdit;
        _pemPassShowButton = new QPushButton;
        // Fix for MAC OSX: PEM pass show button was created bigger, making it same size as other pushbuttons
        _pemPassShowButton->setMaximumWidth(_pemFileBrowseButton->width());
        VERIFY(connect(_pemPassShowButton, SIGNAL(clicked()), this, SLOT(togglePassphraseShowMode())));
        togglePassphraseShowMode();
        _askPemPassCheckBox = new QCheckBox("Ask for passphrase each time");
        _askPemPassCheckBox->setChecked(sslSettings->askPassphrase());
        VERIFY(connect(_askPemPassCheckBox, SIGNAL(toggled(bool)), this, SLOT(on_askPemPassCheckBox_toggle(bool))));

        // Advanced options
        _useAdvancedOptionsCheckBox = new QCheckBox("Advanced Options");
        _useAdvancedOptionsCheckBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        VERIFY(connect(_useAdvancedOptionsCheckBox, SIGNAL(toggled(bool)), this, SLOT(on_useAdvancedOptionsCheckBox_toggle(bool))));
        _crlFileLabel = new QLabel("CRL (Revocation List): ");
        _crlFileLabel->setToolTip(CRL_FILE_HINT);
        _crlFilePathLineEdit = new QLineEdit;
        _crlFileBrowseButton = new QPushButton("...");
        _crlFileBrowseButton->setMaximumWidth(50);
        VERIFY(connect(_crlFileBrowseButton, SIGNAL(clicked()), this, SLOT(on_crlFileBrowseButton_clicked())));
        _allowInvalidHostnamesLabel = new QLabel("Invalid Hostnames: ");
        _allowInvalidHostnamesLabel->setToolTip(ALLOW_INVALID_HOSTNAME_HINT);
        _allowInvalidHostnamesComboBox = new QComboBox;
        _allowInvalidHostnamesComboBox->addItem("Not Allowed");
        _allowInvalidHostnamesComboBox->addItem("Allowed");
        _allowInvalidHostnamesComboBox->setCurrentIndex(sslSettings->allowInvalidHostnames());

        // Layouts
        // Auth. method section
        QGridLayout* gridLayout = new QGridLayout;
        gridLayout->addWidget(_authMethodLabel,                 0 ,0);
        gridLayout->addWidget(_authMethodComboBox,              0 ,1, 1, 2);
        gridLayout->addWidget(_selfSignedInfoStr,               1, 1, 1, 2);
        gridLayout->addWidget(_caFileLabel,                     2, 0);
        gridLayout->addWidget(_caFilePathLineEdit,              2, 1);
        gridLayout->addWidget(_caFileBrowseButton,              2, 2);        
#ifdef _WIN32
        gridLayout->addWidget(new QLabel(""),                   3, 0);
#endif
        // PEM File Section
        gridLayout->addWidget(_usePemFileCheckBox,              4, 0, Qt::AlignTop);
        gridLayout->addWidget(_pemFileInfoStr,                  4, 1, 1, 2);
        gridLayout->addWidget(_pemFileLabel,                    5, 0);
        gridLayout->addWidget(_pemFilePathLineEdit,             5, 1);
        gridLayout->addWidget(_pemFileBrowseButton,             5, 2);
        gridLayout->addWidget(_pemPassLabel,                    6, 0);
        gridLayout->addWidget(_pemPassLineEdit,                 6, 1);
        gridLayout->addWidget(_pemPassShowButton,               6, 2);
        gridLayout->addWidget(_askPemPassCheckBox,              7, 1, 1, 2);
#ifdef _WIN32
        gridLayout->addWidget(new QLabel(""),                   8, 0);        
#endif
        // Advanced section
        gridLayout->addWidget(_useAdvancedOptionsCheckBox,      9, 0, Qt::AlignTop);
        gridLayout->addWidget(_crlFileLabel,                    10, 0);
        gridLayout->addWidget(_crlFilePathLineEdit,             10, 1);
        gridLayout->addWidget(_crlFileBrowseButton,             10, 2);
        gridLayout->addWidget(_allowInvalidHostnamesLabel,      11, 0);
        gridLayout->addWidget(_allowInvalidHostnamesComboBox,   11, 1, Qt::AlignLeft);

        auto mainLayout = new QVBoxLayout;
        mainLayout->setAlignment(Qt::AlignTop);
        mainLayout->addWidget(_useSslCheckBox);
        mainLayout->addLayout(gridLayout);
        setLayout(mainLayout);

        // Load SSL settings to update UI states
        _useSslCheckBox->setChecked(sslSettings->sslEnabled());
        _authMethodComboBox->setCurrentIndex(!sslSettings->allowInvalidCertificates());
        _caFilePathLineEdit->setText(QString::fromStdString(sslSettings->caFile()));
        _usePemFileCheckBox->setChecked(sslSettings->usePemFile());
        _pemFilePathLineEdit->setText(QString::fromStdString(sslSettings->pemKeyFile()));
        _askPemPassCheckBox->setChecked(sslSettings->askPassphrase());
        // Load passphrase only if askPassphrase is false
        if (!sslSettings->askPassphrase())
        {
            _pemPassLineEdit->setText(QString::fromStdString(sslSettings->pemPassPhrase()));
        }
        _useAdvancedOptionsCheckBox->setChecked(sslSettings->useAdvancedOptions());
        _allowInvalidHostnamesComboBox->setCurrentIndex(sslSettings->allowInvalidHostnames());
        _crlFilePathLineEdit->setText(QString::fromStdString(sslSettings->crlFile()));

        // Update UI inter-connected (signal-slot) widget states
        on_authModeComboBox_change(_authMethodComboBox->currentIndex());
        on_usePemFileCheckBox_toggle(_usePemFileCheckBox->isChecked());
        on_askPemPassCheckBox_toggle(_askPemPassCheckBox->isChecked());
        on_useAdvancedOptionsCheckBox_toggle(_useAdvancedOptionsCheckBox->isChecked());

        // Enable/disable all SSL tab widgets
        useSslCheckBoxStateChange(_useSslCheckBox->checkState());

        // Attempt to fix issue for Windows High DPI button height is slightly taller than other widgets 
#ifdef Q_OS_WIN
        _caFileBrowseButton->setMaximumHeight(HighDpiConstants::WIN_HIGH_DPI_BUTTON_HEIGHT);
        _pemFileBrowseButton->setMaximumHeight(HighDpiConstants::WIN_HIGH_DPI_BUTTON_HEIGHT);
        _pemPassShowButton->setMaximumHeight(HighDpiConstants::WIN_HIGH_DPI_BUTTON_HEIGHT);
        _crlFileBrowseButton->setMaximumHeight(HighDpiConstants::WIN_HIGH_DPI_BUTTON_HEIGHT);
#endif

    }

    bool SSLTab::accept()
    {
        saveSslSettings();
        return validate();
    }

    bool SSLTab::sslEnabled() const 
    { 
        return _useSslCheckBox->isChecked(); 
    }

    void SSLTab::clearTab()
    {
        _authMethodComboBox->setCurrentIndex(1);
        _caFilePathLineEdit->clear();
        _usePemFileCheckBox->setChecked(false);
        _pemFilePathLineEdit->clear();
        _pemPassLineEdit->clear();
        _useAdvancedOptionsCheckBox->setChecked(false);
        _allowInvalidHostnamesComboBox->setCurrentIndex(0);
    }

    void SSLTab::setSslOptions(
        int index,
        bool allowInvalidHostnames,
        std::string_view caFile,
        std::string_view certPemFile,
        std::string_view certPemFilePwd
    ) {
        _useSslCheckBox->setChecked(true);
        _authMethodComboBox->setCurrentIndex(index);
        _caFilePathLineEdit->setText(QString::fromStdString(std::string(caFile)));

        if(!certPemFile.empty()) _usePemFileCheckBox->setChecked(true);
        _pemFilePathLineEdit->setText(QString::fromStdString(std::string(certPemFile)));
        _pemPassLineEdit->setText(QString::fromStdString(std::string(certPemFilePwd)));

        if (allowInvalidHostnames) _useAdvancedOptionsCheckBox->setChecked(true);
        _allowInvalidHostnamesComboBox->setCurrentIndex(allowInvalidHostnames);
    }

    void SSLTab::useSslCheckBoxStateChange(int state)
    {
        bool isChecked = static_cast<bool>(state);
        _authMethodLabel->setDisabled(!isChecked);
        _authMethodComboBox->setDisabled(!isChecked);
        _selfSignedInfoStr->setDisabled(!isChecked);
        _caFileLabel->setDisabled(!isChecked);
        _caFilePathLineEdit->setDisabled(!isChecked);
        _caFileBrowseButton->setDisabled(!isChecked);
        _usePemFileCheckBox->setDisabled(!isChecked);
        _pemFileInfoStr->setDisabled(!isChecked);
        _pemFileLabel->setDisabled(!isChecked);
        _pemFilePathLineEdit->setDisabled(!isChecked);
        _pemFileBrowseButton->setDisabled(!isChecked);
        _pemPassLabel->setDisabled(!isChecked);
        _pemPassLineEdit->setDisabled(!isChecked);
        _pemPassShowButton->setDisabled(!isChecked);
        _askPemPassCheckBox->setDisabled(!isChecked);
        _useAdvancedOptionsCheckBox->setDisabled(!isChecked);
        _crlFileLabel->setDisabled(!isChecked);
        _crlFilePathLineEdit->setDisabled(!isChecked);
        _crlFileBrowseButton->setDisabled(!isChecked);
        _allowInvalidHostnamesLabel->setDisabled(!isChecked);
        _allowInvalidHostnamesComboBox->setDisabled(!isChecked);
        if (isChecked)  // Update some widgets only if SSL enabled
        {
            on_usePemFileCheckBox_toggle(_usePemFileCheckBox->isChecked());
            on_askPemPassCheckBox_toggle(_askPemPassCheckBox->isChecked());
        }
    }

    void SSLTab::on_authModeComboBox_change(int index)
    {
        bool const isCaSigned = static_cast<bool>(index);
        _selfSignedInfoStr->setVisible(!isCaSigned);
        _caFileLabel->setVisible(isCaSigned);
        _caFilePathLineEdit->setVisible(isCaSigned);
        _caFileBrowseButton->setVisible(isCaSigned);
    }

    void SSLTab::on_usePemFileCheckBox_toggle(bool isChecked)
    {
        _pemFileInfoStr->setVisible(!isChecked);
        _pemFileLabel->setVisible(isChecked);
        _pemFilePathLineEdit->setVisible(isChecked);
        _pemFileBrowseButton->setVisible(isChecked);
        _pemPassLabel->setVisible(isChecked);
        _pemPassLineEdit->setVisible(isChecked);
        _pemPassShowButton->setVisible(isChecked);
        _askPemPassCheckBox->setVisible(isChecked);
    }

    void SSLTab::on_useAdvancedOptionsCheckBox_toggle(bool isChecked)
    {
        _crlFileLabel->setVisible(isChecked);
        _crlFilePathLineEdit->setVisible(isChecked);
        _crlFileBrowseButton->setVisible(isChecked);
        _allowInvalidHostnamesLabel->setVisible(isChecked);
        _allowInvalidHostnamesComboBox->setVisible(isChecked);
    }

    void SSLTab::on_caFileBrowseButton_clicked()
    {
        QString const& fileName = openFileBrowseDialog(_caFilePathLineEdit->text());
        if (!fileName.isEmpty()) {
            _caFilePathLineEdit->setText(fileName);
        }
    }

    void SSLTab::on_pemKeyFileBrowseButton_clicked()
    {
        QString const& fileName = openFileBrowseDialog(_pemFilePathLineEdit->text());
        if (!fileName.isEmpty()) {
            _pemFilePathLineEdit->setText(fileName);
        }
    }

    void SSLTab::on_crlFileBrowseButton_clicked()
    {
        QString const& fileName = openFileBrowseDialog(_crlFilePathLineEdit->text());
        if (!fileName.isEmpty()) {
            _crlFilePathLineEdit->setText(fileName);
        }
    }

    void SSLTab::togglePassphraseShowMode()
    {
        bool isPassword = _pemPassLineEdit->echoMode() == QLineEdit::Password;
        _pemPassLineEdit->setEchoMode(isPassword ? QLineEdit::Normal : QLineEdit::Password);
        _pemPassShowButton->setIcon(isPassword ? GuiRegistry::instance().showIcon() : GuiRegistry::instance().hideIcon());
    }

    void SSLTab::on_askPemPassCheckBox_toggle(bool checked)
    {
        //if (_usePemFileCheckBox->isChecked())
        //{
            _pemPassLabel->setDisabled(checked);
            _pemPassLineEdit->setDisabled(checked);
            _pemPassShowButton->setDisabled(checked);
            if (checked)
            {
                _pemPassLineEdit->setText("");  // clear passphrase on UI
            }
        //}
    }

    bool SSLTab::validate()
    {
        // Validate existence of files
        auto const& resultAndFileName = checkExistenseOfFiles();
        if (!resultAndFileName.first)
        {
            QString const& nonExistingFile = resultAndFileName.second;
            QMessageBox errorBox;
            errorBox.critical(this, "Error", ("Error: " + nonExistingFile + " file does not exist"));
            errorBox.adjustSize();
            return false;
        }

        return true;
    }

    std::pair<bool,QString> SSLTab::checkExistenseOfFiles() const
    {
        if (_caFilePathLineEdit->isEnabled() && _caFilePathLineEdit->isVisible()) {
            if (!fileExists(_caFilePathLineEdit->text())) {
                return {false, "CA-signed certificate"};
            }
        }

        if (_pemFilePathLineEdit->isVisible() && _pemFilePathLineEdit->isEnabled()) {
            if (!fileExists(_pemFilePathLineEdit->text())) {
                return {false, "PEM Certificate/Key"};
            }
        }

        if (!_crlFilePathLineEdit->text().isEmpty()) {
            if (!fileExists(_crlFilePathLineEdit->text())) {
                return {false, "CRL (Revocation List)"};
            }
        }
        return {true, ""};
    }

    void SSLTab::saveSslSettings() const
    {
        SslSettings* sslSettings = _connSettings->sslSettings();
        sslSettings->enableSSL(_useSslCheckBox->isChecked());
        sslSettings->setAllowInvalidCertificates(!static_cast<bool>(_authMethodComboBox->currentIndex()));
        sslSettings->setCaFile(QtUtils::toStdString(_caFilePathLineEdit->text()));
        sslSettings->setUsePemFile(_usePemFileCheckBox->isChecked());
        sslSettings->setPemKeyFile(QtUtils::toStdString(_pemFilePathLineEdit->text()));
        sslSettings->setAskPassphrase(_askPemPassCheckBox->isChecked());
        // save passphrase only if _askPemPassCheckBox is not checked; otherwise don't save and clear saved passphrase
        if (!_askPemPassCheckBox->isChecked())
        {
            sslSettings->setPemPassPhrase(QtUtils::toStdString(_pemPassLineEdit->text()));
        }
        else
        {
            sslSettings->setPemPassPhrase("");
        }
        sslSettings->setUseAdvancedOptions(_useAdvancedOptionsCheckBox->isChecked());
        sslSettings->setCrlFile(QtUtils::toStdString(_crlFilePathLineEdit->text()));
        sslSettings->setAllowInvalidHostnames(static_cast<bool>(_allowInvalidHostnamesComboBox->currentIndex()));
    }

    QString SSLTab::openFileBrowseDialog(const QString& initialPath)
    {
        QString filePath = initialPath;
        // If user has previously selected a file, initialize file dialog with that file's 
        // path and name; otherwise, use user's home directory.
        if (filePath.isEmpty())
        {
            filePath = QDir::homePath();
        }
        QString fileName = QFileDialog::getOpenFileName(this, tr("Choose File"), filePath);

        // Some strange behaviour at least on Mac happens when you close QFileDialog. Focus switched to a different modal
        // dialog, not the one that was active before openning QFileDialog.
        // http://stackoverflow.com/questions/17998811/window-modal-qfiledialog-pushing-parent-to-background-after-exec
        QApplication::activeModalWidget()->raise();
        QApplication::activeModalWidget()->activateWindow();

        return QDir::toNativeSeparators(fileName);
    }
}