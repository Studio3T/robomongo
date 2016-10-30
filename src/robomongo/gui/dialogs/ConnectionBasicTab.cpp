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

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/ReplicaSetSettings.h"

namespace Robomongo
{
    ConnectionBasicTab::ConnectionBasicTab(ConnectionSettings *settings) :
        _settings(settings)
    {
        const ReplicaSetSettings* const replicaSettings = _settings->replicaSetSettings();

        _typeLabel = new QLabel("Type:");
        _connectionType = new QComboBox;
        _connectionType->addItem(tr("Direct Connection"));
        _connectionType->addItem(tr("Replica Set or Sharded Cluster")); 
        _connectionType->setCurrentIndex(static_cast<int>(_settings->isReplicaSet()));
        VERIFY(connect(_connectionType, SIGNAL(currentIndexChanged(int)), this, SLOT(on_ConnectionTypeChange(int))));
        
        _nameLabel = new QLabel("Name:"); 
        _connectionName = new QLineEdit(QtUtils::toQString(_settings->connectionName()));
        _connInfoLabel = new QLabel("Choose any connection name that will help you to identify this connection.");
        _connInfoLabel->setWordWrap(true);
        _connInfoLabel->setContentsMargins(0, -2, 0, 20);

        _addressLabel = new QLabel("Address:");
        _serverAddress = new QLineEdit(QtUtils::toQString(_settings->serverHost()));
        _colon = new QLabel(":");
        _serverPort = new QLineEdit(QString::number(_settings->serverPort()));
        _serverPort->setFixedWidth(80);
        QRegExp rx("\\d+"); //(0-65554)
        _serverPort->setValidator(new QRegExpValidator(rx, this)); 
        _addInfoLabel = new QLabel("Specify host and port of MongoDB server. Host can be either IPv4, IPv6 or domain name.");
        _addInfoLabel->setWordWrap(true);
        _addInfoLabel->setContentsMargins(0, -2, 0, 20);

        _membersLabel = new QLabel("Members:");
        _members = new QTreeWidget;
        _members->setHeaderHidden(true);
        _members->setIndentation(0);

        if (_settings->isReplicaSet()) 
        {
            if (replicaSettings->members().empty()) 
            {
                auto item = new QTreeWidgetItem;
                item->setText(0, "localhost:27017");
                item->setFlags(item->flags() | Qt::ItemIsEditable);
                _members->addTopLevelItem(item);
            }
            else            
            {
                for (const std::string& str : replicaSettings->members()) 
                {
                    if (!str.empty()) {  // todo: refactor
                        auto item = new QTreeWidgetItem;
                        item->setText(0, QString::fromStdString(str));
                        item->setFlags(item->flags() | Qt::ItemIsEditable);
                        _members->addTopLevelItem(item);
                    }
                }
            }
        }

        int const BUTTON_SIZE = 60;
        _addButton = new QPushButton("+");
        _addButton->setFixedWidth(30);
        _removeButton = new QPushButton("-");
        _removeButton->setFixedWidth(30);
        _discoverButton = new QPushButton("Find Set");
        _discoverButton->setFixedWidth(_discoverButton->sizeHint().width()*0.8);
        VERIFY(connect(_addButton, SIGNAL(clicked()), this, SLOT(on_addButton_clicked())));
        VERIFY(connect(_removeButton, SIGNAL(clicked()), this, SLOT(on_removeButton_clicked())));

        auto hLayout = new QHBoxLayout;
        hLayout->addWidget(_addButton);
        hLayout->addWidget(_removeButton);
        hLayout->addWidget(_discoverButton);
        hLayout->setAlignment(Qt::AlignRight);

        _readPrefLabel = new QLabel("Read Preference:");
        _readPrefLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        _readPreference = new QComboBox;
        _readPreference->addItems(QStringList({ "Primary", "Primary Preferred" }));  //todo
        if (_settings->isReplicaSet()) {
            _readPreference->setCurrentIndex(static_cast<int>(replicaSettings->readPreference()));
        }
        
        QGridLayout *connectionLayout = new QGridLayout;
        connectionLayout->setAlignment(Qt::AlignTop);
        connectionLayout->addWidget(_typeLabel,                     1, 0);
        connectionLayout->addWidget(_connectionType,                1, 1, 1, 3);        
        connectionLayout->addWidget(new QLabel(""),                 2, 0);
        connectionLayout->addWidget(_nameLabel,                     3, 0);
        connectionLayout->addWidget(_connectionName,                3, 1, 1, 3);
        connectionLayout->addWidget(_connInfoLabel,                 4, 1, 1, 3);
        connectionLayout->addWidget(_addressLabel,                  5, 0);
        connectionLayout->addWidget(_serverAddress,                 5, 1);
        connectionLayout->addWidget(_colon,                         5, 2);
        connectionLayout->addWidget(_serverPort,                    5, 3);
        connectionLayout->addWidget(_addInfoLabel,                  6, 1, 1, 3);
        connectionLayout->addWidget(_membersLabel,                  7, 0, Qt::AlignTop);
        connectionLayout->addWidget(_members,                       7, 1, 1, 3);        
        connectionLayout->addLayout(hLayout,                        8, 1, 1, 3, Qt::AlignRight);
        connectionLayout->addWidget(new QLabel(""),                 9, 0);
        connectionLayout->addWidget(_readPrefLabel,                 10, 0);
        connectionLayout->addWidget(_readPreference,                10, 1, 1, 3);        
        connectionLayout->addWidget(new QLabel(""),                 11, 0);

        QVBoxLayout *mainLayout = new QVBoxLayout;
        mainLayout->addLayout(connectionLayout);
        setLayout(mainLayout);

        _connectionName->setFocus();
        on_ConnectionTypeChange(_connectionType->currentIndex());
    }

    void ConnectionBasicTab::accept()
    {
        ReplicaSetSettings* const replicaSettings = _settings->replicaSetSettings();
       
        _settings->setReplicaSet(static_cast<bool>(_connectionType->currentIndex()));
        _settings->setConnectionName(QtUtils::toStdString(_connectionName->text()));
        _settings->setServerHost(QtUtils::toStdString(_serverAddress->text()));
        _settings->setServerPort(_serverPort->text().toInt());
        if (_settings->isReplicaSet()) {
            // save replica members
            std::vector<std::string> members;
            for (int i = 0; i < _members->topLevelItemCount(); ++i)
            {
                QTreeWidgetItem const* item = _members->topLevelItem(i);
                if (!item->text(0).isEmpty()) {
                    members.push_back(item->text(0).toStdString());
                }
            }
            replicaSettings->setMembers(members);
            // save read preference option 
            replicaSettings->setReadPreference(
                static_cast<ReplicaSetSettings::ReadPreference>(_readPreference->currentIndex()));
        }
    }

    void ConnectionBasicTab::on_ConnectionTypeChange(int index)
    {
        bool const isReplica = static_cast<bool>(index);
        
        // Replica set
        _membersLabel->setVisible(isReplica);
        _members->setVisible(isReplica);
        _addButton->setVisible(isReplica);
        _removeButton->setVisible(isReplica);
        _discoverButton->setVisible(isReplica);
        _readPrefLabel->setVisible(isReplica);
        _readPreference->setVisible(isReplica);
        
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
        item->setText(0, "localhost:27017");
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        _members->addTopLevelItem(item);
    }

    void ConnectionBasicTab::on_removeButton_clicked()
    {
        if (_members->topLevelItemCount() <= 0)
            return;

        if (_members->currentItem()) {
            delete _members->currentItem();
        }
        else {
            delete _members->topLevelItem(_members->topLevelItemCount() - 1);
        }
    }
}
