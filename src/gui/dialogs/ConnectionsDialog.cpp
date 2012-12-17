#include <QObject>
#include <QApplication>
#include <QPushButton>
#include <QHBoxLayout>
#include <QHash>
#include <QAction>
#include <QMessageBox>
#include "ConnectionsDialog.h"
#include "dialogs/EditConnectionDialog.h"
#include "GuiRegistry.h"

using namespace Robomongo;

/**
 * @brief Creates dialog
 */
ConnectionsDialog::ConnectionsDialog(SettingsManager *settingsManager) : QDialog()
{
    setWindowIcon(GuiRegistry::instance().connectIcon());

    _settingsManager = settingsManager;
    connect(_settingsManager, SIGNAL(connectionAdded(ConnectionRecordPtr)), this, SLOT(add(ConnectionRecordPtr)));
    connect(_settingsManager, SIGNAL(connectionUpdated(ConnectionRecordPtr)), this, SLOT(update(ConnectionRecordPtr)));
    connect(_settingsManager, SIGNAL(connectionRemoved(ConnectionRecordPtr)), this, SLOT(remove(ConnectionRecordPtr)));

    QAction *addAction = new QAction("&Add", this);
    connect(addAction, SIGNAL(triggered()), this, SLOT(add()));

    QAction *editAction = new QAction("&Edit", this);
    connect(editAction, SIGNAL(triggered()), this, SLOT(edit()));

    QAction *removeAction = new QAction("&Remove", this);
    connect(removeAction, SIGNAL(triggered()), this, SLOT(remove()));

    _listWidget = new QListWidget;
    _listWidget->setViewMode(QListView::ListMode);
    _listWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
    _listWidget->addAction(addAction);
    _listWidget->addAction(editAction);
    _listWidget->addAction(removeAction);
    connect(_listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(accept()));

    QPushButton *addButton = new QPushButton("&Add");
    connect(addButton, SIGNAL(clicked()), this, SLOT(add()));

    QPushButton *editButton = new QPushButton("&Edit");
    connect(editButton, SIGNAL(clicked()), this, SLOT(edit()));

    QPushButton *removeButton = new QPushButton("&Remove");
    connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));

    QPushButton *cancelButton = new QPushButton("&Cancel");
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    QPushButton *connectButton = new QPushButton("C&onnect");
    connectButton->setIcon(GuiRegistry::instance().serverIcon());
    connect(connectButton, SIGNAL(clicked()), this, SLOT(accept()));

    QHBoxLayout *bottomLayout = new QHBoxLayout;
    bottomLayout->addWidget(connectButton, 1, Qt::AlignRight);
    bottomLayout->addWidget(cancelButton);

    QVBoxLayout *firstColumnLayout = new QVBoxLayout;
    firstColumnLayout->addWidget(_listWidget);
    firstColumnLayout->addLayout(bottomLayout);

    QVBoxLayout *secondColumnLayout = new QVBoxLayout;
    secondColumnLayout->setAlignment(Qt::AlignTop);
    secondColumnLayout->addWidget(addButton);
    secondColumnLayout->addWidget(editButton);
    secondColumnLayout->addWidget(removeButton);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addLayout(firstColumnLayout);
    mainLayout->addLayout(secondColumnLayout);

    setWindowTitle("Connections...");
    //setWindowIcon(AppRegistry::instance().serverIcon());

    // Remove help button (?)
    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Populate list with connections
    foreach(ConnectionRecordPtr connectionModel, _settingsManager->connections())
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
    ConnectionRecordPtr newModel(new ConnectionRecord);
    EditConnectionDialog editDialog(newModel);

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
    ConnectionListWidgetItem *currentItem = (ConnectionListWidgetItem *) _listWidget->currentItem();

    // Do nothing if no item selected
    if (currentItem == 0)
        return;

    ConnectionRecordPtr connection = currentItem->connection();
    EditConnectionDialog editDialog(connection);

    // Do nothing if not accepted
    if (editDialog.exec() != QDialog::Accepted)
        return;

    _settingsManager->updateConnection(connection);
}

/**
 * @brief Initiate 'remove' action, usually when user clicked on Remove button
 */
void ConnectionsDialog::remove()
{
    ConnectionListWidgetItem *currentItem = (ConnectionListWidgetItem *)_listWidget->currentItem();

    // Do nothing if no item selected
    if (currentItem == 0)
        return;

    ConnectionRecordPtr connectionModel = currentItem->connection();

    // Ask user
    int answer = QMessageBox::question(this,
        "Connections",
        QString("Really delete \"%1\" connection?").arg(connectionModel->getReadableName()),
        QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

    if (answer != QMessageBox::Yes)
        return;

    _settingsManager->removeConnection(connectionModel);
}

/**
 * @brief Add connection to the list widget
 */
void ConnectionsDialog::add(const ConnectionRecordPtr &connection)
{
    ConnectionListWidgetItem *item = new ConnectionListWidgetItem;
    item->setConnection(connection);
    _listWidget->addItem(item);
    _hash.insert(connection, item);
}

/**
 * @brief Update specified connection (if it exists for this dialog)
 */
void ConnectionsDialog::update(const ConnectionRecordPtr &connection)
{
    ConnectionListWidgetItem *item = _hash.value(connection);
    if (!item)
        return;

    item->setConnection(connection);
}

/**
 * @brief Remove specified connection (if it exists for this dialog)
 */
void ConnectionsDialog::remove(const ConnectionRecordPtr &connection)
{
    QListWidgetItem *item = _hash.value(connection);
    if (!item)
        return;

    delete item;
}
