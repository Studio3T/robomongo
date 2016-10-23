#include "robomongo/gui/dialogs/CreateConnectionDialog.h"

#include <QPushButton>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QApplication>
#include <QSplitter>
#include <QDialogButtonBox>

#include <QLineEdit>
#include <QPixmap>
#include <QLabel>

#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/dialogs/ConnectionsDialog.h"
#include "robomongo/gui/dialogs/ConnectionDiagnosticDialog.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/utils/QtUtils.h"

#include <mongo/client/connection_string.h>
//#include <mongo/base/status_with.h>

namespace Robomongo
{
    /**
    * @brief Constructs dialog with specified connection
    */
    CreateConnectionDialog::CreateConnectionDialog(ConnectionsDialog* parent)
        : QDialog(), _connectionsDialog(parent), _connection(nullptr)
    {
        setWindowTitle("Create New Connection");
        setWindowIcon(GuiRegistry::instance().serverIcon());
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)
        setMinimumWidth(450);

        // Splitter-Left
        auto label01 = new QLabel("Replica Set or Sharded Cluster");
        label01->setStyleSheet("font-weight: bold");
        auto wid1 = new QLabel;
        auto repSetImage = QPixmap("C:\\Users\\gsimsek\\Google Drive\\Cases - Drive\\replica_sets\\icons\\repset.png");
        wid1->setPixmap(repSetImage);
        
        _uriLineEdit = new QLineEdit("mongodb://localhost:27017,localhost:27018,localhost:27019/admin?replicaSet=repset");

        //auto nameLineEdit = new QLineEdit("New Connection");
        //auto nameLay = new QGridLayout;
        ////connectionLayout->setAlignment(Qt::AlignTop);
        //nameLay->setColumnStretch(1, 1);
        ////nameLay->setColumnMinimumWidth(0, _passwordLabel->sizeHint().width() + 5);
        //nameLay->addWidget(new QLabel("Connection Name:"),  0, 0, Qt::AlignLeft);
        //nameLay->addWidget(nameLineEdit,                    0, 1);
        //auto nameWid = new QWidget;
        //nameWid->setLayout(nameLay);

        auto splitterL = new QSplitter;
        splitterL->setOrientation(Qt::Vertical);
        splitterL->addWidget(label01);
        splitterL->addWidget(wid1);
        //splitterL->addWidget(new QLabel(""));
        //splitterL->addWidget(nameWid);
        splitterL->addWidget(new QLabel(""));
        splitterL->addWidget(new QLabel("a) I have a connection string URI:"));
        splitterL->addWidget(_uriLineEdit);
        splitterL->addWidget(new QLabel(""));
        splitterL->addWidget(new QLabel(QString("b) <a style='color: %1' href='create'>Create connection manually</a>").arg("#106CD6")));
        splitterL->addWidget(new QLabel(""));


        // Splitter-Right
        auto label2 = new QLabel("Stand Alone Server");
        label2->setStyleSheet("font-weight: bold");
        auto wid2 = new QLabel;
        auto standAlone = QPixmap("C:\\Users\\gsimsek\\Google Drive\\Cases - Drive\\replica_sets\\icons\\stand_alone.png");
        wid2->setPixmap(standAlone);

        auto splitterR = new QSplitter;
        splitterR->setOrientation(Qt::Vertical);
        splitterR->addWidget(label2);
        splitterR->addWidget(wid2);

        // Splitter layout
        auto splitterLay = new QHBoxLayout;
        splitterLay->addWidget(splitterL);
        //splitterLay->addWidget(splitterR);

        // Button Box
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


        // main layout
        auto mainLayout = new QVBoxLayout;
        mainLayout->addLayout(splitterLay);
        mainLayout->addLayout(bottomLayout);
        setLayout(mainLayout);

        //_basicTab->setFocus();
        adjustSize();

        // Set minimum width - adjustment after adding SSLTab
        setMinimumWidth(550);
    }

    /**
    * @brief Accept() is called when user agree with entered data.
    */
    void CreateConnectionDialog::accept()
    {
        //if (validateAndApply()) {
        //    QDialog::accept();
        //}

        auto uriWithStatus = getUriWithStatus();
        if (!uriWithStatus || !uriWithStatus->isOK()) {
            return;
        }

        auto uri = uriWithStatus->getValue();
        auto connection = new ConnectionSettings(uri);    // todo: refactor
        _connectionsDialog->add(connection);

        QDialog::accept();
    }

    bool CreateConnectionDialog::validateAndApply()
    {
        return true;
    }

    std::unique_ptr<CreateConnectionDialog::MongoURIwithStatus> CreateConnectionDialog::getUriWithStatus()
    {
        const std::string& url = _uriLineEdit->text().toStdString();

        std::unique_ptr<MongoURIwithStatus> uriWithStatus = nullptr;
        try {
            uriWithStatus.reset(new MongoURIwithStatus(mongo::MongoURI::parse(url)));
            if (!uriWithStatus->isOK()) {
                QMessageBox::critical(this, "Error", "Failed to parse connection string URI.\n"
                                      "Reason: " + QString::fromStdString(uriWithStatus->getStatus().reason()));
                return uriWithStatus;
            }
        }
        catch (const std::exception& exception) {
            QMessageBox::critical(this, "Error", "Failed to parse connection string URI.\n"
                                  "Reason: " + QString::fromStdString(exception.what()));
            return nullptr;
        }

        return uriWithStatus;
    }

    /**
    * @brief Test current connection
    */
    void CreateConnectionDialog::testConnection()
    {
        //if (!validateAndApply())
        //    return;

        auto uriWithStatus = getUriWithStatus();
        if (!uriWithStatus || !uriWithStatus->isOK()) {
            return;
        }

        auto uri = uriWithStatus->getValue();
        auto connection = ConnectionSettings(uri);    // todo: refactor
        ConnectionDiagnosticDialog diag(&connection, this);
        if (!diag.continueExec())
            return;

        diag.exec();
    }
}
