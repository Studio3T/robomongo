#include "robomongo/gui/dialogs/ConnectionDialog.h"

#include <QPushButton>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QApplication>
#include <QDialogButtonBox>

#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/dialogs/ConnectionAuthTab.h"
#include "robomongo/gui/dialogs/ConnectionBasicTab.h"
#include "robomongo/gui/dialogs/ConnectionAdvancedTab.h"
#include "robomongo/gui/dialogs/ConnectionSslTab.h"
#ifdef SSH_SUPPORT_ENABLED
#include "robomongo/gui/dialogs/SSHTunnelTab.h"
#endif
#include "robomongo/gui/dialogs/ConnectionDiagnosticDialog.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    /**
     * @brief Constructs dialog with specified connection
     */
    ConnectionDialog::ConnectionDialog(ConnectionSettings *connection) 
        : QDialog(),
        _connection(connection)
    {
        setWindowTitle("Connection Settings");
        setWindowIcon(GuiRegistry::instance().serverIcon());
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)
        setMinimumWidth(450);

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
                
        _basicTab    = new ConnectionBasicTab(_connection);
        _authTab     = new ConnectionAuthTab(_connection);
        _advancedTab = new ConnectionAdvancedTab(_connection);
        _sslTab      = new ConnectionSslTab(_connection);

#ifdef SSH_SUPPORT_ENABLED
        _sshTab = new SshTunnelTab(_connection);
#endif

        tabWidget->addTab(_basicTab,    "Connection");
        tabWidget->addTab(_authTab,     "Authentication");
        tabWidget->addTab(_advancedTab, "Advanced");
        tabWidget->addTab(_sslTab,      "SSL");
#ifdef SSH_SUPPORT_ENABLED
        tabWidget->addTab(_sshTab,      "SSH");
#endif

        QVBoxLayout *mainLayout = new QVBoxLayout;
        mainLayout->addWidget(tabWidget);
        mainLayout->addLayout(bottomLayout);
        setLayout(mainLayout);

        _basicTab->setFocus();
    }

    /**
     * @brief Accept() is called when user agree with entered data.
     */
    void ConnectionDialog::accept()
    {
        if(validateAndApply()){
            QDialog::accept();
        }
    }

    bool ConnectionDialog::validateAndApply()
    {
#ifdef SSH_SUPPORT_ENABLED
        bool isSshAndSsl = _sslTab->isSslSupported() && _sshTab->isSshSupported();
        if (isSshAndSsl) {
            QMessageBox::warning(this, "Invalid Transport", "SSH and SSL cannot be enabled simultaneously. Please uncheck one of them.");
            return false;
        }
#endif
        _basicTab->accept();
        _authTab->accept();
        _advancedTab->accept();
        _sslTab->accept();
#ifdef SSH_SUPPORT_ENABLED
        _sshTab->accept();
#endif
        return true;
    }

    /**
     * @brief Test current connection
     */
    void ConnectionDialog::testConnection()
    {
        if(validateAndApply()){
            ConnectionDiagnosticDialog diag(_connection,this);
            diag.exec();
        }
    }
}
