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
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QMouseEvent>

using namespace Robomongo;

/**
 * @brief Creates dialog
 */
ConnectionsDialog::ConnectionsDialog(SettingsManager *settingsManager) : QDialog()
{
    setWindowIcon(GuiRegistry::instance().connectIcon());
    setWindowTitle("Manage Connections");
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

    _listWidget = new ConnectionsTreeWidget;
    _listWidget->setIndentation(5);

    QStringList colums;
    colums << "Name" << "Address" << "Auth. Database / User";
    _listWidget->setHeaderLabels(colums);
    _listWidget->header()->setResizeMode(0, QHeaderView::Stretch);
    _listWidget->header()->setResizeMode(1, QHeaderView::Stretch);
    _listWidget->header()->setResizeMode(2, QHeaderView::Stretch);

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
    connect(_listWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(accept()));
    connect(_listWidget, SIGNAL(layoutChanged()), this, SLOT(listWidget_layoutChanged()));

//    QPushButton *addButton = new QPushButton("&Add");
//    connect(addButton, SIGNAL(clicked()), this, SLOT(add()));

//    QPushButton *editButton = new QPushButton("&Edit");
//    connect(editButton, SIGNAL(clicked()), this, SLOT(edit()));

//    QPushButton *removeButton = new QPushButton("&Remove");
//    connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));

//    QPushButton *cloneButton = new QPushButton("&Clone");
//    connect(cloneButton, SIGNAL(clicked()), this, SLOT(clone()));

    QPushButton *cancelButton = new QPushButton("&Cancel");
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    QPushButton *connectButton = new QPushButton("C&onnect");
    connectButton->setIcon(GuiRegistry::instance().serverIcon());
    connect(connectButton, SIGNAL(clicked()), this, SLOT(accept()));

    QHBoxLayout *bottomLayout = new QHBoxLayout;
    bottomLayout->addWidget(connectButton, 1, Qt::AlignRight);
    bottomLayout->addWidget(cancelButton);

    QLabel *intro = new QLabel(
    "Here you can manage your connections to MongoDB instances.<br/> You can <a href='create'>create</a>, "
    "<a href='edit'>edit</a>, <a href='remove'>remove</a>, <a href='clone'>clone</a> and reorder connections via drag'n'drop.");
    intro->setWordWrap(true);
    connect(intro, SIGNAL(linkActivated(QString)), this, SLOT(linkActivated(QString)));

    QVBoxLayout *firstColumnLayout = new QVBoxLayout;
    firstColumnLayout->addWidget(intro);
    firstColumnLayout->addWidget(_listWidget, 1);
    firstColumnLayout->addLayout(bottomLayout);

//    QVBoxLayout *secondColumnLayout = new QVBoxLayout;
//    secondColumnLayout->setAlignment(Qt::AlignTop);
//    secondColumnLayout->addWidget(new QLabel); // funny placeholder
//    secondColumnLayout->addWidget(addButton);
//    secondColumnLayout->addWidget(editButton);
//    secondColumnLayout->addWidget(cloneButton);
//    secondColumnLayout->addWidget(removeButton);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addLayout(firstColumnLayout, 1);

    // Populate list with connections
    foreach(ConnectionSettings *connectionModel, _settingsManager->connections())
        add(connectionModel);

    // Highlight first item
    if (_listWidget->topLevelItemCount() > 0)
        _listWidget->setCurrentItem(_listWidget->topLevelItem(0));
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
    QList<ConnectionSettings *> items;
    for(int i = 0; i < count; i++)
    {
        ConnectionListWidgetItem * item = (ConnectionListWidgetItem *) _listWidget->topLevelItem(i);
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
    item->setIcon(0, GuiRegistry::instance().serverIcon());

    _listWidget->addTopLevelItem(item);
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
    QTreeWidgetItem *item = _hash.value(connection);
    if (!item)
        return;

    delete item;
}

/**
 * @brief Attach ConnectionSettings to this item
 */
void ConnectionListWidgetItem::setConnection(ConnectionSettings *connection)
{
    setText(0, connection->connectionName());
    setText(1, connection->getFullAddress());

    if (connection->hasEnabledPrimaryCredential()) {
        QString authString = QString("%1 / %2")
            .arg(connection->primaryCredential()->databaseName())
            .arg(connection->primaryCredential()->userName());

        setText(2, authString);
        setIcon(2, GuiRegistry::instance().yesMarkIcon());
    } else {
        setIcon(2, QIcon());
        setText(2, "");
    }

    _connection = connection;
}


ConnectionsTreeWidget::ConnectionsTreeWidget()
{
    setDragDropMode(QAbstractItemView::InternalMove);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setDragEnabled(true);
    setAcceptDrops(true);
}
/*
void ConnectionsTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void ConnectionsTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}
*/

void ConnectionsTreeWidget::dropEvent(QDropEvent *event)
{
    QTreeWidget::dropEvent(event);
    emit layoutChanged();
/*    event->accept();
    QTreeWidgetItem *dropingOn = itemAt(event->pos());
    int dropingIndex = indexOfTopLevelItem(dropingOn);
    takeTopLevelItem(indexOfTopLevelItem(draggingItem));
    int index = indexOfTopLevelItem(dropingOn);

    if(index < dropingIndex)
        index++;

    insertTopLevelItem(index, draggingItem);*/
}

/*
void ConnectionsTreeWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton) {
        draggingItem = itemAt(event->pos());
        QDrag *drag = new QDrag(this);
        drag->exec(Qt::MoveAction | Qt::CopyAction, Qt::CopyAction);
        event->accept();
   }
}
*/
