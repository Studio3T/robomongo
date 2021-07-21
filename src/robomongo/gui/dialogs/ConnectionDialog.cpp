#include "robomongo/gui/dialogs/ConnectionDialog.h"

#include <QPushButton>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QApplication>
#include <QDialogButtonBox>
#include <QSettings>

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/dialogs/ConnectionAuthTab.h"
#include "robomongo/gui/dialogs/ConnectionBasicTab.h"
#include "robomongo/gui/dialogs/ConnectionAdvancedTab.h"
#include "robomongo/gui/dialogs/SSHTunnelTab.h"
#include "robomongo/gui/dialogs/SSLTab.h"
#include "robomongo/gui/dialogs/ConnectionDiagnosticDialog.h"

namespace Robomongo
{
    /**
     * @brief Constructs dialog with specified connection
     */
    ConnectionDialog::ConnectionDialog(ConnectionSettings *connection, QWidget *parent /* = nullptr */)
        : QDialog(parent), _connection(connection)
    {
        setWindowTitle("Connection Settings");
        setWindowIcon(GuiRegistry::instance().serverIcon());
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)

        QPushButton *testButton = new QPushButton("&Test");
        testButton->setIcon(qApp->style()->standardIcon(QStyle::SP_MessageBoxInformation));
        VERIFY(connect(testButton, SIGNAL(clicked()), this, SLOT(testConnection())));

        QHBoxLayout *bottomLayout = new QHBoxLayout;
        bottomLayout->addWidget(testButton, 1, Qt::AlignLeft);
        QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
        VERIFY(connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept())));
        VERIFY(connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject())));
        bottomLayout->addWidget(buttonBox);

        QTabWidget *tabWidget = new QTabWidget;
                
        _basicTab    = new ConnectionBasicTab(_connection, this);
        _authTab     = new ConnectionAuthTab(_connection);
        _advancedTab = new ConnectionAdvancedTab(_connection);
        _sshTab      = new SshTunnelTab(_connection);
        _sslTab      = new SSLTab(_connection);

        tabWidget->addTab(_basicTab,    "Connection");
        tabWidget->addTab(_authTab,     "Authentication");
        tabWidget->addTab(_sshTab,      "SSH");
        tabWidget->addTab(_sslTab,      "TLS");
        tabWidget->addTab(_advancedTab, "Advanced");

        QVBoxLayout *mainLayout = new QVBoxLayout;
        mainLayout->addWidget(tabWidget);
        mainLayout->addLayout(bottomLayout);
        setLayout(mainLayout);

        _basicTab->setFocus();
        adjustSize();

#ifdef _WIN32
        setMinimumWidth(500);
#elif __APPLE__
        setMinimumWidth(660);
#elif __linux__        
        setMinimumWidth(900);
#endif

        restoreWindowSettings();
    }

    /**
     * @brief Accept() is called when user agree with entered data.
     */
    void ConnectionDialog::accept()
    {
        saveWindowSettings();

        if (validateAndApply())
            QDialog::accept();
    }

    void ConnectionDialog::reject()
    {
        saveWindowSettings();
        QDialog::reject();
    }

    void ConnectionDialog::closeEvent(QCloseEvent *event)
    {
        saveWindowSettings();
        QWidget::closeEvent(event);
    }

    void ConnectionDialog::setAuthTab(
        QString const& db, 
        QString const& username, 
        QString const& pwd, 
        AuthMechanism authMech
    ) {
        _authTab->setAuthTab(db, username, pwd, authMech);
    }

    void ConnectionDialog::setDefaultDb(QString const& defaultDb)
    {
        _advancedTab->setDefaultDb(defaultDb);
    }

    void ConnectionDialog::toggleSshSupport(bool isReplicaSet)
    {
        if (!_sshTab) 
            return;
        
        _sshTab->setDisabled(isReplicaSet);
        _sshTab->toggleSshCheckboxToolTip(isReplicaSet);
    }

    void ConnectionDialog::clearConnAuthTab()
    {
        _authTab->clearTab();
    }

    void ConnectionDialog::clearSslTab()
    {
        _sslTab->clearTab();
    }

    void ConnectionDialog::setSslTab(
        int index,
        bool allowInvalidHostnames,
        std::string_view caFile,
        std::string_view certPemFile,
        std::string_view certPemFilePwd
    ) {
        _sslTab->setSslOptions(index, allowInvalidHostnames, caFile, certPemFile, certPemFilePwd);
    }

    void ConnectionDialog::restoreWindowSettings()
    {
        QSettings settings("3T", "Robomongo");
        resize(settings.value("ConnectionDialog/size").toSize());
    }

    void ConnectionDialog::saveWindowSettings() const
    {
        QSettings settings("3T", "Robomongo");
        settings.setValue("ConnectionDialog/size", size());
    }

    bool ConnectionDialog::validateAndApply()
    {
        _authTab->accept();
        _advancedTab->accept();

        if (!_basicTab->accept() || !_sshTab->accept() || !_sslTab->accept())
            return false;

        return true;
    }

    /**
     * @brief Test current connection
     */
    void ConnectionDialog::testConnection()
    {
        if (!validateAndApply())
            return;

        ConnectionDiagnosticDialog diag(_connection, this);
        if (!diag.continueExec())
            return;

        diag.exec();
    }
}
