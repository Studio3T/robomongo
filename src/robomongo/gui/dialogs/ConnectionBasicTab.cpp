#include "robomongo/gui/dialogs/ConnectionBasicTab.h"

#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QRegExpValidator>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QComboBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QApplication>
#include <QDesktopWidget>

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/ReplicaSetSettings.h"
#include "robomongo/gui/dialogs/ConnectionDialog.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/utils/GuiConstants.h"

#include "mongo/client/mongo_uri.h"

namespace Robomongo
{
    ConnectionBasicTab::ConnectionBasicTab(ConnectionSettings *settings, ConnectionDialog *connectionDialog) :
        _settings(settings), _connectionDialog(connectionDialog)
    {
        _typeLabel = new QLabel("Type:");
        _connectionType = new QComboBox;
        _connectionType->addItem(tr("Direct Connection"));
        _connectionType->addItem(tr("Replica Set")); 
        _connectionType->setCurrentIndex(static_cast<int>(_settings->isReplicaSet()));
        VERIFY(connect(_connectionType, SIGNAL(currentIndexChanged(int)), 
                        this, SLOT(on_ConnectionTypeChange(int)))
        );
        
        _nameLabel = new QLabel("Name:"); 
        _connectionName = new QLineEdit(QtUtils::toQString(_settings->connectionName()));
        _connInfoLabel = new QLabel("Choose any connection name that will help you to identify this connection.");
        _connInfoLabel->setWordWrap(true);

        _addressLabel = new QLabel("Address:");
        _serverAddress = new QLineEdit(QtUtils::toQString(_settings->serverHost()));
        _colon = new QLabel(":");
        _serverPort = new QLineEdit(QString::number(_settings->serverPort()));
        _serverPort->setFixedWidth(80);
        QRegExp rx("\\d+"); //(0-65554)
        _serverPort->setValidator(new QRegExpValidator(rx, this)); 
        _addInfoLabel = new QLabel("Specify host and port of MongoDB server. Host can be either IPv4, IPv6 or domain name.");
        _addInfoLabel->setWordWrap(true);

        _membersLabel = new QLabel("Members:");
        _membersLabel->setFixedWidth(_membersLabel->sizeHint().width());
        _members = new QTreeWidget;
        _members->setHeaderHidden(true);
        _members->setIndentation(0);
#ifdef _WIN32
        auto lineHeight = _members->fontMetrics().height();
        _members->setFixedHeight(lineHeight * 8);
#endif
        VERIFY(connect(_members, SIGNAL(itemChanged(QTreeWidgetItem*, int)), 
                       this, SLOT(on_replicaMemberItemEdit(QTreeWidgetItem*, int))));

        if (_settings->isReplicaSet() && _settings->replicaSetSettings()->members().size() > 0) {
            for (const std::string& str : _settings->replicaSetSettings()->members()) {
                if (!str.empty()) {
                    auto item = new QTreeWidgetItem;
                    item->setText(0, QString::fromStdString(str));
                    item->setFlags(item->flags() | Qt::ItemIsEditable);
                    _members->addTopLevelItem(item);
                }
            }
            // To fix strange MAC alignment issue
#ifdef __APPLE__
            auto lineHeight = _members->fontMetrics().height();
            _members->setFixedHeight(lineHeight * 8);
#endif
        }
        else {  // No members
            auto item = new QTreeWidgetItem;
            item->setText(0, "localhost:27017");
            item->setFlags(item->flags() | Qt::ItemIsEditable);
            _members->addTopLevelItem(item);
        }

        int const BUTTON_SIZE = 60;
        _addButton = new QPushButton;
        _addButton->setIcon(GuiRegistry::instance().plusIcon());
        _removeButton = new QPushButton;
        _removeButton->setIcon(GuiRegistry::instance().minusIcon());
        VERIFY(connect(_addButton, SIGNAL(clicked()), this, SLOT(on_addButton_clicked())));
        VERIFY(connect(_removeButton, SIGNAL(clicked()), this, SLOT(on_removeButton_clicked())));

        _minusPlusButtonBox = new QDialogButtonBox(this);
        _minusPlusButtonBox->setOrientation(Qt::Horizontal);
#ifdef _WIN32
        _minusPlusButtonBox->addButton(_addButton, QDialogButtonBox::NoRole);
        _minusPlusButtonBox->addButton(_removeButton, QDialogButtonBox::NoRole);
#else
        _minusPlusButtonBox->addButton(_removeButton, QDialogButtonBox::NoRole);
        _minusPlusButtonBox->addButton(_addButton, QDialogButtonBox::NoRole);
#endif
        _setNameLabel = new QLabel("Set Name:");
        _setNameEdit = new QLineEdit(QString::fromStdString(_settings->replicaSetSettings()->setNameUserEntered()));

        auto fakeSpacer = new QLabel("");
        auto hline = new QFrame();
        hline->setFrameShape(QFrame::HLine);
        hline->setFrameShadow(QFrame::Sunken);
        _uriEdit = new QLineEdit();
        _uriEdit->setPlaceholderText("Import connection details from MongoDB URI connection string");
        _uriButton = new QPushButton("From URI");
#ifdef _WIN32
        _uriButton->setMaximumHeight(HighDpiConstants::WIN_HIGH_DPI_BUTTON_HEIGHT);
        _uriButton->setMinimumWidth(60);
#else   // MacOS
        _uriButton->setMaximumHeight(HighDpiConstants::MACOS_HIGH_DPI_BUTTON_HEIGHT);   
        _uriButton->setMaximumWidth(90);
#endif        
        VERIFY(connect(_uriButton, SIGNAL(clicked()), this, SLOT(on_uriButton_clicked())));

        auto connLayout = new QGridLayout;
        connLayout->setVerticalSpacing(8);
        connLayout->setAlignment(Qt::AlignTop);
        connLayout->addWidget(_typeLabel,                     1, 0);
        connLayout->addWidget(_connectionType,                1, 1, 1, 3);
        connLayout->addWidget(_nameLabel,                     3, 0);
        connLayout->addWidget(_connectionName,                3, 1, 1, 3);
        connLayout->addWidget(_addressLabel,                  5, 0);
        connLayout->addWidget(_serverAddress,                 5, 1);
        connLayout->addWidget(_colon,                         5, 2);
        connLayout->addWidget(_serverPort,                    5, 3);
        connLayout->addWidget(_addInfoLabel,                  6, 1, 1, 3);
        connLayout->addWidget(_membersLabel,                  7, 0, Qt::AlignTop);
        connLayout->addWidget(_members,                       7, 1, 1, 3);
        connLayout->addWidget(_minusPlusButtonBox,            8, 3, Qt::AlignRight | Qt::AlignTop);
        connLayout->addWidget(_setNameLabel,                  9, 0);
        connLayout->addWidget(_setNameEdit,                   9, 1, 1, 3, Qt::AlignTop);
        connLayout->addWidget(fakeSpacer,                    10, 0);
        connLayout->addWidget(hline,                         11, 0, 1, 4);
        connLayout->addWidget(_uriButton,                    13, 0);
        connLayout->addWidget(_uriEdit,                      13, 1, 1, 3);

        connLayout->setRowStretch(10, 1);        
#ifdef __APPLE__
        connLayout->setRowMinimumHeight(11, 20);
#endif

        auto mainLayout = new QVBoxLayout;
        mainLayout->addLayout(connLayout);
        setLayout(mainLayout);
#ifdef __APPLE__
        mainLayout->setContentsMargins(-1, -1, -1, 10);
#endif

        _connectionName->setFocus();
        on_ConnectionTypeChange(_connectionType->currentIndex());        
    }

    bool ConnectionBasicTab::accept()
    {
        _settings->setReplicaSet(static_cast<bool>(_connectionType->currentIndex()));
        _settings->setConnectionName(QtUtils::toStdString(_connectionName->text()));

        if (_settings->isReplicaSet() && _members->topLevelItemCount() == 0) {
            QMessageBox::critical(this, "Error", "Replica set members cannot be empty. "  
                                                 "Please enter at least one member.");
            return false;
        }
        
        // Check and warn if there is duplicate member or 
        // if any of the replica set member items does not contain ":" character between hostname and port.
        if (_settings->isReplicaSet() && _members->topLevelItemCount() > 1) {
            QStringList members;
            for (int i = 0; i < _members->topLevelItemCount(); ++i) {
                QTreeWidgetItem const* item = _members->topLevelItem(i);
                QStringList const hostAndPort = item->text(0).split(":");
                if (hostAndPort.size() < 2) {
                    QMessageBox::critical(this, "Error", "Replica set member items must all contain ':' between"
                                                " hostname and port.");
                    return false;
                }
                if (!item->text(0).isEmpty()) 
                    members.push_back(item->text(0));
            }
            if (members.size() > 1) {
                if (members.removeDuplicates() > 0) {
                    QMessageBox::critical(this, "Error", "Please remove duplicate member, two replica"
                                                " set members cannot have the same hostname and port.");
                    return false;
                }
            }
        }

        // Save to settings
        if (_settings->isReplicaSet() && _members->topLevelItemCount() > 0) {
            QStringList const hostAndPort = _members->topLevelItem(0)->text(0).split(":");
            _settings->setServerHost(hostAndPort[0].toStdString());
            _settings->setServerPort(hostAndPort[1].toInt());
        }
        else {  // Single server
            _settings->setServerHost(QtUtils::toStdString(_serverAddress->text()));
            _settings->setServerPort(_serverPort->text().toInt());
        }

        if (_settings->isReplicaSet()) {
            // Save replica members
            std::vector<std::string> members;
            for (int i = 0; i < _members->topLevelItemCount(); ++i)
            {
                QTreeWidgetItem const* item = _members->topLevelItem(i);
                if (!item->text(0).isEmpty()) 
                    members.push_back(item->text(0).toStdString());
            }
            _settings->replicaSetSettings()->setMembers(members);
            _settings->replicaSetSettings()->setSetNameUserEntered(_setNameEdit->text().toStdString());
            // Clear cached set name
            _settings->replicaSetSettings()->setCachedSetName("");
        }

        return true;
    }

    void ConnectionBasicTab::clearTab()
    {
        _connectionType->setCurrentIndex(0);
        _connectionName->setText("New Connection");
        _serverAddress->clear();
        _serverPort->clear();
        _members->clear();
        _setNameEdit->clear();
    }

    void ConnectionBasicTab::on_ConnectionTypeChange(int index)
    {
        bool const isReplica = static_cast<bool>(index);

        _connectionDialog->toggleSshSupport(isReplica);
        
        // Replica set
        _membersLabel->setVisible(isReplica);
        _members->setVisible(isReplica);
        _minusPlusButtonBox->setVisible(isReplica);
        _setNameLabel->setVisible(isReplica);
        _setNameEdit->setVisible(isReplica);
            
        // Direct Connection
        _addressLabel->setVisible(!isReplica);
        _serverAddress->setVisible(!isReplica);
        _serverPort->setVisible(!isReplica);
        _colon->setVisible(!isReplica);
        _addInfoLabel->setVisible(!isReplica);
    }

    void ConnectionBasicTab::deleteItem()
    {
        delete _members->currentItem(); // todo: refactor
    }

    void ConnectionBasicTab::on_addButton_clicked()
    {
        auto item = new QTreeWidgetItem;

        // Make member addition little smarter than expected
        if (_members->topLevelItemCount() < 1) {
            item->setText(0, "localhost:27017");
        }
        else {  // Add the next member using last entered hostname and incremented port by one
            QString const& lastMember = _members->topLevelItem(_members->topLevelItemCount()-1)->text(0);
            QStringList const& hostAndPort = lastMember.split(':');
            if (hostAndPort.size() == 2) {  
                auto const& hostName = hostAndPort[0];
                auto const& port = hostAndPort[1].toInt();
                item->setText(0, hostName + ':' + QString::number(port + 1));
            }
            else {  
                item->setText(0, "localhost:" + QString::number(_members->topLevelItemCount() + 27017));
            }
        }

        item->setFlags(item->flags() | Qt::ItemIsEditable);
        _members->addTopLevelItem(item);
    }

    void ConnectionBasicTab::on_removeButton_clicked()
    {
        if (_members->topLevelItemCount() <= 0)
            return;

        if (_members->currentItem()) 
            delete _members->currentItem();
        else
            delete _members->topLevelItem(_members->topLevelItemCount() - 1);
    }

    void ConnectionBasicTab::on_replicaMemberItemEdit(QTreeWidgetItem* item, int column)
    {
        if (!item)
            return;

        auto str = item->text(0);

        // Remove white spaces
        str = str.simplified();
        str.remove(" ");

        // Remove item from tree widget if it has empty text
        if (str.isEmpty()) {
            delete item;
            return;
        }

        // Force port as integer
        QStringList const& hostAndPort = str.split(':');
        if (hostAndPort.size() >= 2) {
            auto const& hostName = hostAndPort[0];
            auto portStr = hostAndPort[1];
            portStr.remove(QRegExp("[^\\d]"));
            str = hostName + ':' + QString::number(portStr.toInt());
        }
        else 
            str += ":27017";

        item->setText(0, str);
    }

    void ConnectionBasicTab::on_uriButton_clicked()
    {
        // Parse Mongo URI
        QString uriStr = _uriEdit->text().simplified();
        uriStr.replace(" ", "");
        auto const statusWithURI = mongo::MongoURI::parse(uriStr.toStdString());
        if (!statusWithURI.isOK()) {
            QMessageBox errorBox;
            errorBox.critical(
                this, "Error", ("MongoDB URI:\n" + statusWithURI.getStatus().toString()).c_str()
            );
            errorBox.show();
            return;
        }
        auto const mongoUri = statusWithURI.getValue();

        // Clear tabs
        clearTab();
        _connectionDialog->clearConnAuthTab();
        _connectionDialog->clearSslTab();

        // Set Basic (this) Tab
        auto const isReplicaSet = mongoUri.type() == mongo::ConnectionString::ConnectionType::SET;
        if(isReplicaSet) {
            _connectionType->setCurrentIndex(1);    // Switch to Replica Set
            for (auto const& hostAndPort : mongoUri.getServers()) {
                auto host = QString::fromStdString(hostAndPort.host());
                host.endsWith('.') ? host.remove(host.size()-1, 1) : "no-op";
                auto const newHostAndPort = host + ':' + QString::number(hostAndPort.port());
                auto item = new QTreeWidgetItem;
                item->setText(0, newHostAndPort);
                item->setFlags(item->flags() | Qt::ItemIsEditable);
                _members->addTopLevelItem(item);
            }
            _setNameEdit->setText(QString::fromStdString(mongoUri.getSetName()));
        }
        else {  // Standalone
            _connectionType->setCurrentIndex(0);
            _serverAddress->setText(QString::fromStdString(mongoUri.getServers()[0].host()));
            _serverPort->setText(QString::number(mongoUri.getServers()[0].port()));
        }
        // Set Auth Tab
        auto const user = QString::fromStdString(mongoUri.getUser());
        auto const pwd = QString::fromStdString(mongoUri.getPassword());
        auto const authDb = QString::fromStdString(mongoUri.getAuthenticationDatabase());
        auto const authMechanism = mongoUri.getOption("authMechanism").get_value_or("");
        if(!user.isEmpty()) _connectionDialog->setAuthTab(
            authDb, user, pwd, authMechanismFromStr(authMechanism)
        );
        // Set SSL Tab
        if(mongoUri.getSSLMode() == mongo::transport::ConnectSSLMode::kEnableSSL) {
            auto tlsAllowInvalidCertificates = mongoUri.getOption("tlsAllowInvalidCertificates");
            int const authMethodIndex = tlsAllowInvalidCertificates.get_value_or("") == "true" ? 0 : 1;
            auto const caFile = mongoUri.getOption("tlsCAFile").get_value_or("");
            auto const certKeyFile = mongoUri.getOption("tlsCertificateKeyFile").get_value_or("");
            auto const certKeyFilePwd = 
                mongoUri.getOption("tlsCertificateKeyFilePassword").get_value_or("");
            auto const allowInvalidHostnames = 
                mongoUri.getOption("tlsAllowInvalidHostnames").get_value_or("") == "true";

            _connectionDialog->setSslTab(
                authMethodIndex, allowInvalidHostnames, caFile, certKeyFile, certKeyFilePwd
            );
        }
        // Advanced Tab
        _connectionDialog->setDefaultDb(QString::fromStdString(mongoUri.getDatabase()));
    }
}