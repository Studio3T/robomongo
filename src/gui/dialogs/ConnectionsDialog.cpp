#include <QObject>
#include <QApplication>
#include <QPushButton>
#include <QHBoxLayout>
#include <QHash>
#include <QAction>
#include <QMessageBox>
#include "ConnectionsDialog.h"
#include "dialogs/ConnectionDialog.h"
#include "settings/ConnectionSettings.h"
#include "settings/SettingsManager.h"
#include "GuiRegistry.h"
#include <QLabel>

using namespace Robomongo;

/**
 * @brief Creates dialog
 */
ConnectionsDialog::ConnectionsDialog(SettingsManager *settingsManager) : QDialog()
{
    setWindowIcon(GuiRegistry::instance().connectIcon());
    setWindowTitle("Connections...");
    // Remove help button (?)
    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    _settingsManager = settingsManager;
    connect(_settingsManager, SIGNAL(connectionAdded(ConnectionSettings *)), this, SLOT(add(ConnectionSettings*)));
    connect(_settingsManager, SIGNAL(connectionUpdated(ConnectionSettings *)), this, SLOT(update(ConnectionSettings*)));
    connect(_settingsManager, SIGNAL(connectionRemoved(ConnectionSettings *)), this, SLOT(remove(ConnectionSettings*)));

    QAction *addAction = new QAction("&Add", this);
    connect(addAction, SIGNAL(triggered()), this, SLOT(add()));

    QAction *editAction = new QAction("&Edit", this);
    connect(editAction, SIGNAL(triggered()), this, SLOT(edit()));

    QAction *cloneAction = new QAction("&Clone", this);
    connect(cloneAction, SIGNAL(triggered()), this, SLOT(clone()));

    QAction *removeAction = new QAction("&Remove", this);
    connect(removeAction, SIGNAL(triggered()), this, SLOT(remove()));

    _listWidget = new QListWidget;
    _listWidget->setViewMode(QListView::ListMode);
    _listWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
    _listWidget->addAction(addAction);
    _listWidget->addAction(editAction);
    _listWidget->addAction(cloneAction);
    _listWidget->addAction(removeAction);
    _listWidget->setSelectionMode(QAbstractItemView::SingleSelection); // single item can be draged or droped
    _listWidget->setDragEnabled(true);
    _listWidget->setDragDropMode(QAbstractItemView::InternalMove);
    _listWidget->setFixedWidth(330);
    connect(_listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(accept()));
    connect(_listWidget->model(), SIGNAL(layoutChanged()), this, SLOT(listWidget_layoutChanged()));

    QPushButton *addButton = new QPushButton("&Add");
    connect(addButton, SIGNAL(clicked()), this, SLOT(add()));

    QPushButton *editButton = new QPushButton("&Edit");
    connect(editButton, SIGNAL(clicked()), this, SLOT(edit()));

    QPushButton *removeButton = new QPushButton("&Remove");
    connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));

    QPushButton *cloneButton = new QPushButton("&Clone");
    connect(cloneButton, SIGNAL(clicked()), this, SLOT(clone()));

    QPushButton *cancelButton = new QPushButton("&Cancel");
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    QPushButton *connectButton = new QPushButton("C&onnect");
    connectButton->setIcon(GuiRegistry::instance().serverIcon());
    connect(connectButton, SIGNAL(clicked()), this, SLOT(accept()));

    QHBoxLayout *bottomLayout = new QHBoxLayout;
    bottomLayout->addWidget(connectButton, 1, Qt::AlignRight);
    bottomLayout->addWidget(cancelButton);

    QVBoxLayout *firstColumnLayout = new QVBoxLayout;
    firstColumnLayout->addWidget(_listWidget, 1);
    firstColumnLayout->addLayout(bottomLayout);

    QVBoxLayout *secondColumnLayout = new QVBoxLayout;
    secondColumnLayout->setAlignment(Qt::AlignTop);
    secondColumnLayout->addWidget(addButton);
    secondColumnLayout->addWidget(editButton);
    secondColumnLayout->addWidget(cloneButton);
    secondColumnLayout->addWidget(removeButton);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addLayout(firstColumnLayout, 1);
    mainLayout->addLayout(secondColumnLayout);

    // Populate list with connections
    foreach(ConnectionSettings *connectionModel, _settingsManager->connections())
        add(connectionModel);
}

/**
 * @brief This function is called when user clicks on "Connect" button.
 */
void ConnectionsDialog::accept()
{
    ConnectionListWidgetItem *currentItem = (ConnectionListWidgetItem *) _listWidget->currentItem();

    // Do nothing if no item selected
    if (currentItem == 0)
        return;

    _selectedConnection = currentItem->connection();

    close();
    QDialog::accept();
}

/**
 * @brief Initiate 'add' action, usually when user clicked on Add button
 */
void ConnectionsDialog::add()
{
    ConnectionSettings *newModel = new ConnectionSettings();
    ConnectionDialog editDialog(newModel);

    // Do nothing if not accepted
    if (editDialog.exec() != QDialog::Accepted)
        return;

    _settingsManager->addConnection(newModel);
    _listWidget->setFocus();
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
    _settingsManager->updateConnection(connection);

    // on linux focus is lost - we need to activate connections dialog
    activateWindow();
}

/**
 * @brief Initiate 'remove' action, usually when user clicked on Remove button
 */
void ConnectionsDialog::remove()
{
    ConnectionListWidgetItem *currentItem =
        (ConnectionListWidgetItem *)_listWidget->currentItem();

    // Do nothing if no item selected
    if (currentItem == 0)
        return;

    ConnectionSettings *connectionModel = currentItem->connection();

    // Ask user
    int answer = QMessageBox::question(this,
        "Connections",
        QString("Really delete \"%1\" connection?").arg(connectionModel->getReadableName()),
        QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

    if (answer != QMessageBox::Yes)
        return;

    _settingsManager->removeConnection(connectionModel);
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
    QString newConnectionName = QString("Copy of %1").arg(connection->connectionName());
    connection->setConnectionName(newConnectionName);

    ConnectionDialog editDialog(connection);

    // Cleanup newly created connection and return, if not accepted.
    if (editDialog.exec() != QDialog::Accepted) {
        delete connection;
        return;
    }

    // Now connection will be owned by SettingsManager
    _settingsManager->addConnection(connection);
}

/**
 * @brief Handles ListWidget layoutChanged() signal
 */
void ConnectionsDialog::listWidget_layoutChanged()
{
    int count = _listWidget->count();
    QList<ConnectionSettings *> items;
    for(int i = 0; i < count; i++)
    {
        ConnectionListWidgetItem * item = (ConnectionListWidgetItem *) _listWidget->item(i);
        items.append(item->connection());
    }

    _settingsManager->reorderConnections(items);
}

/**
 * @brief Add connection to the list widget
 */
void ConnectionsDialog::add(ConnectionSettings *connection)
{
    ConnectionListWidgetItem *item = new ConnectionListWidgetItem(connection);
    item->setIcon(GuiRegistry::instance().serverIcon());

    _listWidget->addItem(item);
    _hash.insert(connection, item);
}

/**
 * @brief Update specified connection (if it exists for this dialog)
 */
void ConnectionsDialog::update(ConnectionSettings *connection)
{
    ConnectionListWidgetItem *item = _hash.value(connection);
    if (!item)
        return;

    item->setConnection(connection);
}

/**
 * @brief Remove specified connection (if it exists for this dialog)
 */
void ConnectionsDialog::remove(ConnectionSettings *connection)
{
    QListWidgetItem *item = _hash.value(connection);
    if (!item)
        return;

    delete item;
}

/**
 * @brief Attach ConnectionSettings to this item
 */
void ConnectionListWidgetItem::setConnection(ConnectionSettings *connection)
{
    setText(connection->connectionName());
    _connection = connection;
}
