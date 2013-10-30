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
        ConnectionListWidgetItem(ConnectionSettings *connection) { setConnection(connection); }

        /**
         * @brief Returns attached ConnectionSettings.
         */
        ConnectionSettings *connection() { return _connection; }

        /**
         * @brief Attach ConnectionSettings to this item
         */
        void setConnection(ConnectionSettings *connection)
        {
            setText(0, QtUtils::toQString(connection->connectionName()));
            setText(1, QtUtils::toQString(connection->getFullAddress()));

            if (connection->hasEnabledPrimaryCredential()) {
                QString authString = QString("%1 / %2").arg(QtUtils::toQString(connection->primaryCredential()->databaseName())).arg(QtUtils::toQString(connection->primaryCredential()->userName()));
                setText(2, authString);
                setIcon(2, GuiRegistry::instance().keyIcon());
            } else {
                setIcon(2, QIcon());
                setText(2, "");
            }

            _connection = connection;
            setIcon(0, GuiRegistry::instance().serverIcon());
        }

    private:
        ConnectionSettings *_connection;
    };

    /**
     * @brief Creates dialog
     */
    ConnectionsDialog::ConnectionsDialog(SettingsManager *settingsManager, QWidget *parent) 
        : QDialog(parent), _settingsManager(settingsManager)
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
            "<a href='create'>Create</a>, "
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
            ConnectionSettings *connectionModel = *it;
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
        ConnectionDialog editDialog(newModel);

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
            (ConnectionListWidgetItem *) _listWidget->currentItem();

        // Do nothing if no item selected
        if (currentItem == 0)
            return;

        ConnectionSettings *connection = currentItem->connection();
        boost::scoped_ptr<ConnectionSettings> clonedConnection(connection->clone());
        ConnectionDialog editDialog(clonedConnection.get());

        // Do nothing if not accepted
        if (editDialog.exec() != QDialog::Accepted) {
            // on linux focus is lost - we need to activate connections dialog
            activateWindow();
            return;
        }

        connection->apply(editDialog.connection());       

        // on linux focus is lost - we need to activate connections dialog
        activateWindow();

        int size = _connectionItems.size();
        for (int i=0; i<size; ++i)
        {
            ConnectionListWidgetItem *item = _connectionItems[i];
            if (_connectionItems[i]->connection() == connection) {
                item->setConnection(connection);
                break;
            }
        }        
    }

    /**
     * @brief Initiate 'remove' action, usually when user clicked on Remove button
     */
    void ConnectionsDialog::remove()
    {
        ConnectionListWidgetItem *currentItem =
            (ConnectionListWidgetItem *)_listWidget->currentItem();

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

        _settingsManager->removeConnection(connectionModel);
        delete currentItem;
    }

    void ConnectionsDialog::clone()
    {
        ConnectionListWidgetItem *currentItem =
            (ConnectionListWidgetItem *) _listWidget->currentItem();

        // Do nothing if no item selected
        if (currentItem == 0)
            return;

        // Clone connection
        ConnectionSettings *connection = currentItem->connection()->clone();
        std::string newConnectionName="Copy of "+connection->connectionName();

        connection->setConnectionName(newConnectionName);

        ConnectionDialog editDialog(connection);

        // Cleanup newly created connection and return, if not accepted.
        if (editDialog.exec() != QDialog::Accepted) {
            delete connection;
            return;
        }

        // Now connection will be owned by SettingsManager
        _settingsManager->addConnection(connection);
        add(connection);
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
            ConnectionListWidgetItem * item = (ConnectionListWidgetItem *) _listWidget->topLevelItem(i);
            if (item->childCount() > 0) {
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
    void ConnectionsDialog::add(ConnectionSettings *connection)
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
