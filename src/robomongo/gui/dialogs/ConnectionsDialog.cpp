#include "robomongo/gui/dialogs/ConnectionsDialog.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QAction>
#include <QMessageBox>
#include <QLabel>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QTreeWidgetItem>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/utils/QtUtils.h"

#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/dialogs/ConnectionDialog.h"
#include "robomongo/gui/dialogs/ReplicaSetDialog.h"

namespace Robomongo
{
    
    /**
     * @brief Simple ListWidgetItem that has several convenience methods.
     */
    class ConnectionListWidgetItem : public QTreeWidgetItem
    {
    public:
        /**
         * @brief Creates ConnectionListWidgetItem with specified ConnectionSettings
         */
        ConnectionListWidgetItem(IConnectionSettingsBase *connection): _connection(connection) { refreshFields(); }

        /**
         * @brief Returns attached ConnectionSettings.
         */
        IConnectionSettingsBase *connection() { return _connection; }
        /**
         * @brief Attach ConnectionSettings to this item
         */
        void refreshFields()
        {
            setText(0, QtUtils::toQString(_connection->connectionName()));              
            IConnectionSettingsBase::ConnectionType conType = _connection->connectionType();
            if(conType == IConnectionSettingsBase::REPLICASET){
                ReplicasetConnectionSettings *set = dynamic_cast<ReplicasetConnectionSettings*>(_connection); 
                VERIFY(set);

                // Populate list with connections                
                ReplicasetConnectionSettings::ServerContainerType connections = set->servers();
                QtUtils::clearChildItems(this);
                for (ReplicasetConnectionSettings::ServerContainerType::const_iterator it = connections.begin(); it != connections.end(); ++it) {
                    IConnectionSettingsBase *connection = *it;
                    QTreeWidgetItem::addChild(new ConnectionListWidgetItem(connection));
                }
                setText(1, QtUtils::toQString(set->getFullAddress()));
                setIcon(0, GuiRegistry::instance().replicaSetIcon());
            }
            else if(conType == IConnectionSettingsBase::DIRECT){
                ConnectionSettings *con = dynamic_cast<ConnectionSettings*>(_connection);
                VERIFY(con);

                setText(1, QtUtils::toQString(con->getFullAddress()));
                CredentialSettings primCred = con->primaryCredential();
                if (primCred.isValidAndEnabled()) {
                    CredentialSettings::CredentialInfo info = primCred.info();
                    QString authString = QString("%1 / %2").arg(QtUtils::toQString(info._databaseName)).arg(QtUtils::toQString(info._userName));
                    setText(2, authString);
                    setIcon(2, GuiRegistry::instance().keyIcon());
                } else {
                    setIcon(2, QIcon());
                    setText(2, "");
                }
                setIcon(0, GuiRegistry::instance().serverIcon());
            }
        }

    private:
        IConnectionSettingsBase *_connection;
    };

    /**
     * @brief Creates dialog
     */
    ConnectionsDialog::ConnectionsDialog(SettingsManager *settingsManager, QWidget *parent) 
        : QDialog(parent), _settingsManager(settingsManager), _selectedConnection(NULL)
    {
        setWindowIcon(GuiRegistry::instance().connectIcon());
        setWindowTitle("MongoDB Connections");

        // Remove help button (?)
        setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

        QAction *addAction = new QAction("&Add...", this);
        VERIFY(connect(addAction, SIGNAL(triggered()), this, SLOT(add())));

        QAction *editAction = new QAction("&Edit...", this);
        VERIFY(connect(editAction, SIGNAL(triggered()), this, SLOT(edit())));

        QAction *cloneAction = new QAction("&Clone...", this);
        VERIFY(connect(cloneAction, SIGNAL(triggered()), this, SLOT(clone())));

        QAction *removeAction = new QAction("&Remove...", this);
        VERIFY(connect(removeAction, SIGNAL(triggered()), this, SLOT(remove())));

        _listWidget = new ConnectionsTreeWidget;
        GuiRegistry::instance().setAlternatingColor(_listWidget);
        _listWidget->setIndentation(5);

        QStringList colums;
        colums << "Name" << "Address" << "Auth. Database / User";
        _listWidget->setHeaderLabels(colums);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        _listWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        _listWidget->header()->setSectionResizeMode(1, QHeaderView::Stretch);
        _listWidget->header()->setSectionResizeMode(2, QHeaderView::Stretch);
#endif
        //_listWidget->setViewMode(QListView::ListMode);
        _listWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
        _listWidget->addAction(addAction);
        _listWidget->addAction(editAction);
        _listWidget->addAction(cloneAction);
        _listWidget->addAction(removeAction);
        _listWidget->setIndentation(15);
        _listWidget->setSelectionMode(QAbstractItemView::SingleSelection); // single item can be draged or droped
        _listWidget->setDragEnabled(true);
        _listWidget->setDragDropMode(QAbstractItemView::InternalMove);
        _listWidget->setMinimumHeight(300);
        _listWidget->setMinimumWidth(630);
        VERIFY(connect(_listWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(accept())));
        VERIFY(connect(_listWidget, SIGNAL(layoutChanged()), this, SLOT(listWidget_layoutChanged())));

        QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
        buttonBox->button(QDialogButtonBox::Save)->setIcon(GuiRegistry::instance().serverIcon());
        buttonBox->button(QDialogButtonBox::Save)->setText("C&onnect");
        VERIFY(connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept())));
        VERIFY(connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject())));

        QHBoxLayout *bottomLayout = new QHBoxLayout;
        bottomLayout->addWidget(buttonBox);

        QLabel *intro = new QLabel(
            "<a href='create'>Create</a>, <a href='create replica set'>Create Replicaset</a>,"
            "<a href='edit'>edit</a>, <a href='remove'>remove</a>, <a href='clone'>clone</a> or reorder connections via drag'n'drop.");
        intro->setWordWrap(true);
        VERIFY(connect(intro, SIGNAL(linkActivated(QString)), this, SLOT(linkActivated(QString))));

        QVBoxLayout *firstColumnLayout = new QVBoxLayout;
        firstColumnLayout->addWidget(intro);
        firstColumnLayout->addWidget(_listWidget, 1);
        firstColumnLayout->addLayout(bottomLayout);

        QHBoxLayout *mainLayout = new QHBoxLayout(this);
        mainLayout->addLayout(firstColumnLayout, 1);

        // Populate list with connections
        SettingsManager::ConnectionSettingsContainerType connections = _settingsManager->connections();
        for (SettingsManager::ConnectionSettingsContainerType::const_iterator it = connections.begin(); it != connections.end(); ++it) {
            IConnectionSettingsBase *connectionModel = *it;
            add(connectionModel);
        }

        // Highlight first item
        if (_listWidget->topLevelItemCount() > 0)
            _listWidget->setCurrentItem(_listWidget->topLevelItem(0));

        _listWidget->setFocus();
    }

    /**
     * @brief This function is called when user clicks on "Connect" button.
     */
    void ConnectionsDialog::accept()
    {
        ConnectionListWidgetItem *currentItem = (ConnectionListWidgetItem *) _listWidget->currentItem();

        // Do nothing if no item selected
        if (!currentItem)
            return;

        _selectedConnection = currentItem->connection();

        QDialog::accept();
    }

    void ConnectionsDialog::linkActivated(const QString &link)
    {
        if (link == "create")
            add();
        else if(link == "create replica set")
            addReplicaSet();
        else if (link == "edit")
            edit();
        else if (link == "remove")
            remove();
        else if (link == "clone")
            clone();
    }

    /**
     * @brief Initiate 'add' action, usually when user clicked on Add button
     */
    void ConnectionsDialog::add()
    {
        ConnectionSettings *newModel = new ConnectionSettings();
        ConnectionDialog editDialog(newModel, this);

        // Do nothing if not accepted
        if (editDialog.exec() != QDialog::Accepted) {
            delete newModel;
            return;
        }

        _settingsManager->addConnection(newModel);
        _listWidget->setFocus();
        add(newModel);
    }

    void ConnectionsDialog::addReplicaSet()
    {
        ReplicasetConnectionSettings *newModel = new ReplicasetConnectionSettings();
        ReplicasetDialog editDialog(newModel, this);

        // Do nothing if not accepted
        if (editDialog.exec() != QDialog::Accepted) {
            delete newModel;
            return;
        }

        _settingsManager->addConnection(newModel);
        _listWidget->setFocus();
        add(newModel);
    }

    /**
     * @brief Initiate 'edit' action, usually when user clicked on Edit button
     */
    void ConnectionsDialog::edit()
    {
        ConnectionListWidgetItem *currentItem =
            dynamic_cast<ConnectionListWidgetItem *>(_listWidget->currentItem());

        // Do nothing if no item selected
        if (!currentItem)
            return;

        IConnectionSettingsBase::ConnectionType conType = currentItem->connection()->connectionType();

        if(conType == IConnectionSettingsBase::DIRECT){
            ConnectionSettings *curCon = dynamic_cast<ConnectionSettings*>(currentItem->connection());
            VERIFY(curCon);

            ConnectionSettings connection(*curCon);
            ConnectionDialog editDialog(&connection, this);

            // Do nothing if not accepted
            if (editDialog.exec() == QDialog::Accepted) {
                *curCon = connection;
                currentItem->refreshFields();
            }
        }
        else if(conType == IConnectionSettingsBase::REPLICASET){
            ReplicasetConnectionSettings *set = dynamic_cast<ReplicasetConnectionSettings*>(currentItem->connection());
            VERIFY(set);

            ReplicasetConnectionSettings connection(*set);
            ReplicasetDialog editDialog(&connection, this);

            // Do nothing if not accepted
            if (editDialog.exec() == QDialog::Accepted) {
                *set = connection;
                currentItem->refreshFields();
            }
        }        
    }

    /**
     * @brief Initiate 'remove' action, usually when user clicked on Remove button
     */
    void ConnectionsDialog::remove()
    {
        ConnectionListWidgetItem *currentItem =
            dynamic_cast<ConnectionListWidgetItem *>(_listWidget->currentItem());

        // Do nothing if no item selected
        if (!currentItem)
            return;

        IConnectionSettingsBase *connectionModel = currentItem->connection();

        // Ask user
        int answer = QMessageBox::question(this,
            "Connections",
            QString("Really delete \"%1\" connection?").arg(QtUtils::toQString(connectionModel->getReadableName())),
            QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

        if (answer != QMessageBox::Yes)
            return;

        ConnectionListWidgetItem *maybeReplica =
            dynamic_cast<ConnectionListWidgetItem *>(_listWidget->currentItem()->parent());
        
        if(maybeReplica){
            ReplicasetConnectionSettings *set = dynamic_cast<ReplicasetConnectionSettings*>(maybeReplica->connection());
            VERIFY(set);
            
            set->removeServer(currentItem->connection());
        }

        _settingsManager->removeConnection(connectionModel);
        delete currentItem;
    }

    void ConnectionsDialog::clone()
    {
        ConnectionListWidgetItem *currentItem =
            (ConnectionListWidgetItem *) _listWidget->currentItem();

        // Do nothing if no item selected
        if (!currentItem)
            return;        
        
        IConnectionSettingsBase::ConnectionType conType = currentItem->connection()->connectionType();

        if(conType == IConnectionSettingsBase::DIRECT){
            ConnectionSettings *curCon = dynamic_cast<ConnectionSettings*>(currentItem->connection());
            VERIFY(curCon);

            // Clone connection
            ConnectionSettings *connection = new ConnectionSettings(*curCon);
            std::string newConnectionName="Copy of "+connection->connectionName();

            connection->setConnectionName(newConnectionName);

            ConnectionDialog editDialog(connection, this);

            // Cleanup newly created connection and return, if not accepted.
            if (editDialog.exec() != QDialog::Accepted) {
                delete connection;
                return;
            }

            // Now connection will be owned by SettingsManager
            _settingsManager->addConnection(connection);
            add(connection);
        }
        else if(conType == IConnectionSettingsBase::REPLICASET){
            ReplicasetConnectionSettings *set = dynamic_cast<ReplicasetConnectionSettings*>(currentItem->connection());
            VERIFY(set);

            // Clone connection
            ReplicasetConnectionSettings *connection = new ReplicasetConnectionSettings(*set);
            std::string newConnectionName="Copy of "+connection->connectionName();

            connection->setConnectionName(newConnectionName);

            ReplicasetDialog editDialog(connection, this);

            // Cleanup newly created connection and return, if not accepted.
            if (editDialog.exec() != QDialog::Accepted) {
                delete connection;
                return;
            }

            // Now connection will be owned by SettingsManager
            _settingsManager->addConnection(connection);
            add(connection);
        }
        
    }

    /**
     * @brief Handles ListWidget layoutChanged() signal
     */
    void ConnectionsDialog::listWidget_layoutChanged()
    {
        int count = _listWidget->topLevelItemCount();

        // Make childrens toplevel again. This is a bad, but quickiest item reordering
        // implementation.
        for(int i = 0; i < count; i++)
        {
            ConnectionListWidgetItem * item = dynamic_cast<ConnectionListWidgetItem *>(_listWidget->topLevelItem(i));
            if (item && item->childCount() > 0) {
                ConnectionListWidgetItem *childItem = (ConnectionListWidgetItem *) item->child(0);
                item->removeChild(childItem);
                _listWidget->insertTopLevelItem(++i, childItem);
                _listWidget->setCurrentItem(childItem);
                break;
            }
        }

        count = _listWidget->topLevelItemCount();
        SettingsManager::ConnectionSettingsContainerType items;
        for(int i = 0; i < count; i++)
        {
            ConnectionListWidgetItem * item = (ConnectionListWidgetItem *) _listWidget->topLevelItem(i);
            items.push_back(item->connection());
        }

        _settingsManager->reorderConnections(items);
    }

    /**
     * @brief Add connection to the list widget
     */
    void ConnectionsDialog::add(IConnectionSettingsBase *connection)
    {
        ConnectionListWidgetItem *item = new ConnectionListWidgetItem(connection);
        _listWidget->addTopLevelItem(item);
        _connectionItems.push_back(item);
    }

    ConnectionsTreeWidget::ConnectionsTreeWidget()
    {
        setDragDropMode(QAbstractItemView::InternalMove);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setDragEnabled(true);
        setAcceptDrops(true);
    }

    void ConnectionsTreeWidget::dropEvent(QDropEvent *event)
    {
        QTreeWidget::dropEvent(event);
        emit layoutChanged();
    }
}
