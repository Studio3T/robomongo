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


#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/ReplicaSetSettings.h"
#include "robomongo/gui/GuiRegistry.h"

namespace Robomongo
{
    ConnectionBasicTab::ConnectionBasicTab(ConnectionSettings *settings) :
        _settings(settings)
    {
        _typeLabel = new QLabel("Type:");
        _connectionType = new QComboBox;
        _connectionType->addItem(tr("Direct Connection"));
        _connectionType->addItem(tr("Replica Set")); 
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
#ifndef _WIN32
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

        _setNameLabel = new QLabel("Set Name:<br><i><font color=\"gray\">(Optional)</font></i>");
        _setNameEdit = new QLineEdit(QString::fromStdString(_settings->replicaSetSettings()->setNameUserEntered()));
        auto _optionalLabel = new QLabel("<i><font color=\"gray\">(Optional)</font></i>");
        
        auto connectionLayout = new QGridLayout;
        connectionLayout->setVerticalSpacing(10);
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
        connectionLayout->addWidget(_minusPlusButtonBox,            8, 3, Qt::AlignRight | Qt::AlignTop);
        connectionLayout->addWidget(new QLabel(""),                 9, 0);
        connectionLayout->addWidget(_setNameLabel,                  10, 0/*,  Qt::AlignBottom*/);
        connectionLayout->addWidget(_setNameEdit,                   10, 1, 1, 3, Qt::AlignTop);
        //connectionLayout->addWidget(_optionalLabel,                 11, 1, 1, 3, Qt::AlignLeft | Qt::AlignTop);
        connectionLayout->addWidget(new QLabel(""),                 12, 0);

        auto mainLayout = new QVBoxLayout;
        mainLayout->addLayout(connectionLayout);
        setLayout(mainLayout);

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

    void ConnectionBasicTab::on_ConnectionTypeChange(int index)
    {
        bool const isReplica = static_cast<bool>(index);
        
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
}
