#include "robomongo/gui/dialogs/ReplicaSetDialog.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QAction>
#include <QMessageBox>
#include <QLabel>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QTreeWidgetItem>
#include <QLineEdit>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/utils/QtUtils.h"

#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/dialogs/ConnectionDialog.h"

namespace Robomongo
{
    
    /**
     * @brief Simple ListWidgetItem that has several convenience methods.
     */
    class ConnectionSettingsListWidgetItem : public QTreeWidgetItem
    {
    public:
        /**
         * @brief Creates ConnectionListWidgetItem with specified ConnectionSettings
         */
        ConnectionSettingsListWidgetItem(ConnectionSettings *connection): _connection(connection) { refreshFields(); }

        /**
         * @brief Returns attached ConnectionSettings.
         */
        ConnectionSettings *connection() { return _connection; }
        /**
         * @brief Attach ConnectionSettings to this item
         */
        void refreshFields()
        {
            setText(0, QtUtils::toQString(_connection->connectionName()));
            setText(1, QtUtils::toQString(_connection->getFullAddress()));

            CredentialSettings primCred = _connection->primaryCredential();
            if (primCred.isValidAnEnabled()) {
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

    private:
        ConnectionSettings *_connection;
    };

    /**
     * @brief Creates dialog
     */
    ReplicasetDialog::ReplicasetDialog(ReplicasetConnectionSettings *connection, QWidget *parent) 
        : QDialog(parent), _repConnection(connection)
    {
        setWindowIcon(GuiRegistry::instance().connectIcon());
        setWindowTitle("Replicaset Connections");
        
        QHBoxLayout *nameL = new QHBoxLayout;
        nameL->addWidget(new QLabel("Name:"));
        _nameConnection = new QLineEdit(QtUtils::toQString(_repConnection->connectionName()));
        nameL->addWidget(_nameConnection);

        QHBoxLayout *replicaL = new QHBoxLayout;
        replicaL->addWidget(new QLabel("Replicaset Name:"));
        _replicasetName = new QLineEdit(QtUtils::toQString(_repConnection->replicaName()));
        replicaL->addWidget(_replicasetName);

        setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

        QAction *addAction = new QAction("&Add...", this);
        VERIFY(connect(addAction, SIGNAL(triggered()), this, SLOT(add())));

        QAction *editAction = new QAction("&Edit...", this);
        VERIFY(connect(editAction, SIGNAL(triggered()), this, SLOT(edit())));

        QAction *cloneAction = new QAction("&Clone...", this);
        VERIFY(connect(cloneAction, SIGNAL(triggered()), this, SLOT(clone())));

        QAction *removeAction = new QAction("&Remove...", this);
        VERIFY(connect(removeAction, SIGNAL(triggered()), this, SLOT(remove())));

        _listWidget = new ReplicaSetConnectionsTreeWidget;
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
        _listWidget->setSelectionMode(QAbstractItemView::SingleSelection); // single item can be draged or droped
        _listWidget->setDragEnabled(true);
        _listWidget->setDragDropMode(QAbstractItemView::InternalMove);
        _listWidget->setMinimumHeight(300);
        _listWidget->setMinimumWidth(630);
        VERIFY(connect(_listWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(accept())));
        VERIFY(connect(_listWidget, SIGNAL(layoutChanged()), this, SLOT(listWidget_layoutChanged())));

        QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
        VERIFY(connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept())));
        VERIFY(connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject())));

        QHBoxLayout *bottomLayout = new QHBoxLayout;
        bottomLayout->addWidget(buttonBox);

        QLabel *intro = new QLabel(
            "<a href='create'>Create</a>,"
            "<a href='edit'>edit</a>, <a href='remove'>remove</a>, <a href='clone'>clone</a> or reorder connections via drag'n'drop.");
        intro->setWordWrap(true);
        VERIFY(connect(intro, SIGNAL(linkActivated(QString)), this, SLOT(linkActivated(QString))));

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->addLayout(nameL);
        mainLayout->addLayout(replicaL);
        mainLayout->addWidget(intro);
        mainLayout->addWidget(_listWidget, 1);
        mainLayout->addLayout(bottomLayout);
        
        // Populate list with connections
        ReplicasetConnectionSettings::ServerContainerType connections = _repConnection->servers();
        for (ReplicasetConnectionSettings::ServerContainerType::const_iterator it = connections.begin(); it != connections.end(); ++it) {
            ConnectionSettings *connection = *it;
            add(connection);
        }

        // Highlight first item
        if (_listWidget->topLevelItemCount() > 0)
            _listWidget->setCurrentItem(_listWidget->topLevelItem(0));

        _listWidget->setFocus();
    }

    /**
     * @brief This function is called when user clicks on "Connect" button.
     */
    void ReplicasetDialog::accept()
    {
        _repConnection->setConnectionName(QtUtils::toStdString(_nameConnection->text()));
        _repConnection->setReplicaName(QtUtils::toStdString(_replicasetName->text()));
        _repConnection->clearServers();

        for (ConnectionListItemContainerType::const_iterator it = _connectionItems.begin(); it != _connectionItems.end(); ++it) {
            ConnectionSettingsListWidgetItem *item = dynamic_cast<ConnectionSettingsListWidgetItem*>(*it);
            if(item){
                _repConnection->addServer(item->connection());
            }
        }
        QDialog::accept();
    }

    void ReplicasetDialog::linkActivated(const QString &link)
    {
        if (link == "create")
            add();
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
    void ReplicasetDialog::add()
    {
        ConnectionSettings *newModel = new ConnectionSettings();
        ConnectionDialog editDialog(newModel);

        // Do nothing if not accepted
        if (editDialog.exec() != QDialog::Accepted) {
            delete newModel;
            return;
        }

        _listWidget->setFocus();
        add(newModel);
    }

    /**
     * @brief Initiate 'edit' action, usually when user clicked on Edit button
     */
    void ReplicasetDialog::edit()
    {
        ConnectionSettingsListWidgetItem *currentItem =
            dynamic_cast<ConnectionSettingsListWidgetItem *>(_listWidget->currentItem());

        // Do nothing if no item selected
        if (!currentItem)
            return;

        ConnectionSettings connection(*currentItem->connection());
        ConnectionDialog editDialog(&connection);

        // Do nothing if not accepted
        if (editDialog.exec() == QDialog::Accepted) {
            *currentItem->connection() = connection;
            currentItem->refreshFields();
        }       
    }

    /**
     * @brief Initiate 'remove' action, usually when user clicked on Remove button
     */
    void ReplicasetDialog::remove()
    {
        ConnectionSettingsListWidgetItem *currentItem =
            dynamic_cast<ConnectionSettingsListWidgetItem *>(_listWidget->currentItem());

        // Do nothing if no item selected
        if (!currentItem)
            return;

        ConnectionSettings *connectionModel = currentItem->connection();

        // Ask user
        int answer = QMessageBox::question(this,
            "Connections",
            QString("Really delete \"%1\" connection?").arg(QtUtils::toQString(connectionModel->getReadableName())),
            QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

        if (answer != QMessageBox::Yes)
            return;
    
        ConnectionListItemContainerType::const_iterator it = std::find(_connectionItems.begin(),_connectionItems.end(),currentItem);
        if(it!=_connectionItems.end()){
            _connectionItems.erase(it);
        }
        delete currentItem;
    }

    void ReplicasetDialog::clone()
    {
        ConnectionSettingsListWidgetItem *currentItem =
            dynamic_cast<ConnectionSettingsListWidgetItem *>(_listWidget->currentItem());

        // Do nothing if no item selected
        if (!currentItem)
            return;

        // Clone connection
        ConnectionSettings *connection = new ConnectionSettings(*currentItem->connection());
        std::string newConnectionName="Copy of "+connection->connectionName();

        connection->setConnectionName(newConnectionName);

        ConnectionDialog editDialog(connection);

        // Cleanup newly created connection and return, if not accepted.
        if (editDialog.exec() != QDialog::Accepted) {
            delete connection;
            return;
        }

        add(connection);        
    }

    /**
     * @brief Handles ListWidget layoutChanged() signal
     */
    void ReplicasetDialog::listWidget_layoutChanged()
    {
       
    }

    /**
     * @brief Add connection to the list widget
     */
    void ReplicasetDialog::add(ConnectionSettings *connection)
    {
        ConnectionSettingsListWidgetItem *item = new ConnectionSettingsListWidgetItem(connection);
        _listWidget->addTopLevelItem(item);
        _connectionItems.push_back(item);
    }

    ReplicaSetConnectionsTreeWidget::ReplicaSetConnectionsTreeWidget()
    {
        setDragDropMode(QAbstractItemView::InternalMove);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setDragEnabled(true);
        setAcceptDrops(true);
    }

    void ReplicaSetConnectionsTreeWidget::dropEvent(QDropEvent *event)
    {
        QTreeWidget::dropEvent(event);
        emit layoutChanged();
    }
}
