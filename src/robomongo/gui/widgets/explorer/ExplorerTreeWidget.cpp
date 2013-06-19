#include "robomongo/gui/widgets/explorer/ExplorerTreeWidget.h"

#include <QContextMenuEvent>
#include <QMenu>
#include <QApplication>
#include <QtGui>
#include <QMessageBox>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/MongoFunction.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/widgets/explorer/ExplorerServerTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseCategoryTreeItem.h"
#include "robomongo/gui/dialogs/DocumentTextEditor.h"
#include "robomongo/gui/dialogs/FunctionTextEditor.h"
#include "robomongo/gui/dialogs/CreateDatabaseDialog.h"
#include "robomongo/gui/dialogs/CreateUserDialog.h"
#include "robomongo/gui/widgets/explorer/ExplorerUserTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerFunctionTreeItem.h"

#include "robomongo/shell/db/json.h"


using namespace Robomongo;

ExplorerTreeWidget::ExplorerTreeWidget(QWidget *parent) : QTreeWidget(parent)
{
#if defined(Q_OS_MAC)
    setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setObjectName("explorerTree");

    QAction *disconnectAction = new QAction("Disconnect", this);
    disconnectAction->setIconText("Disconnect");
    connect(disconnectAction, SIGNAL(triggered()), SLOT(ui_disconnectServer()));

    QAction *refreshAction = new QAction("Refresh", this);
    refreshAction->setIcon(qApp->style()->standardIcon(QStyle::SP_BrowserReload));
    connect(refreshAction, SIGNAL(triggered()), SLOT(ui_refreshServer()));

    QAction *openShellAction = new QAction("Open Shell", this);
    openShellAction->setIcon(GuiRegistry::instance().mongodbIcon());
    connect(openShellAction, SIGNAL(triggered()), SLOT(ui_openShell()));

    QAction *serverHostInfo = new QAction("Host Info", this);
    connect(serverHostInfo, SIGNAL(triggered()), SLOT(ui_serverHostInfo()));

    QAction *serverStatus = new QAction("Server Status", this);
    connect(serverStatus, SIGNAL(triggered()), SLOT(ui_serverStatus()));

    QAction *serverVersion = new QAction("MongoDB Version", this);
    connect(serverVersion, SIGNAL(triggered()), SLOT(ui_serverVersion()));

    QAction *showLog = new QAction("Show Log", this);
    connect(showLog, SIGNAL(triggered()), SLOT(ui_showLog()));

    QAction *refreshServer = new QAction("Refresh", this);
    connect(refreshServer, SIGNAL(triggered()), SLOT(ui_refreshServer()));

    QAction *createDatabase = new QAction("Create Database", this);
    connect(createDatabase, SIGNAL(triggered()), SLOT(ui_createDatabase()));

    _serverContextMenu = new QMenu(this);
    _serverContextMenu->addAction(openShellAction);
    _serverContextMenu->addAction(refreshServer);
    _serverContextMenu->addSeparator();
    _serverContextMenu->addAction(createDatabase);
    _serverContextMenu->addAction(serverStatus);
    _serverContextMenu->addAction(serverHostInfo);
    _serverContextMenu->addAction(serverVersion);
    _serverContextMenu->addSeparator();
    _serverContextMenu->addAction(showLog);
    _serverContextMenu->addAction(disconnectAction);

    QAction *openDbShellAction = new QAction("Open Shell", this);
    openDbShellAction->setIcon(GuiRegistry::instance().mongodbIcon());
    connect(openDbShellAction, SIGNAL(triggered()), SLOT(ui_dbOpenShell()));

    QAction *dbStats = new QAction("Database Statistics", this);
    connect(dbStats, SIGNAL(triggered()), SLOT(ui_dbStatistics()));

    QAction *dbDrop = new QAction("Drop Database", this);
    connect(dbDrop, SIGNAL(triggered()), SLOT(ui_dbDrop()));

    QAction *dbRepair = new QAction("Repair Database", this);
    connect(dbRepair, SIGNAL(triggered()), SLOT(ui_dbRepair()));

    QAction *refreshDatabase = new QAction("Refresh", this);
    connect(refreshDatabase, SIGNAL(triggered()), SLOT(ui_refreshDatabase()));

    _databaseContextMenu = new QMenu(this);
    _databaseContextMenu->addAction(openDbShellAction);
    _databaseContextMenu->addAction(refreshDatabase);
    _databaseContextMenu->addSeparator();
    _databaseContextMenu->addAction(dbStats);
    _databaseContextMenu->addSeparator();
    _databaseContextMenu->addAction(dbRepair);
    _databaseContextMenu->addAction(dbDrop);

    QAction *addDocument = new QAction("Insert Document", this);
    connect(addDocument, SIGNAL(triggered()), SLOT(ui_addDocument()));

    QAction *updateDocument = new QAction("Update Documents", this);
    connect(updateDocument, SIGNAL(triggered()), SLOT(ui_updateDocument()));

    QAction *removeDocument = new QAction("Remove Documents", this);
    connect(removeDocument, SIGNAL(triggered()), SLOT(ui_removeDocument()));

    QAction *removeAllDocuments = new QAction("Remove All Documents", this);
    connect(removeAllDocuments, SIGNAL(triggered()), SLOT(ui_removeAllDocuments()));

    QAction *addIndex = new QAction("Add Index", this);
    connect(addIndex, SIGNAL(triggered()), SLOT(ui_addIndex()));

    QAction *dropIndex = new QAction("Drop Index", this);
    connect(dropIndex, SIGNAL(triggered()), SLOT(ui_dropIndex()));

    QAction *reIndex = new QAction("Rebuild Indexes", this);
    connect(reIndex, SIGNAL(triggered()), SLOT(ui_reIndex()));

    QAction *collectionStats = new QAction("Statistics", this);
    connect(collectionStats, SIGNAL(triggered()), SLOT(ui_collectionStatistics()));

    QAction *storageSize = new QAction("Storage Size", this);
    connect(storageSize, SIGNAL(triggered()), SLOT(ui_storageSize()));

    QAction *totalIndexSize = new QAction("Total Index Size", this);
    connect(totalIndexSize, SIGNAL(triggered()), SLOT(ui_totalIndexSize()));

    QAction *totalSize = new QAction("Total Size", this);
    connect(totalSize, SIGNAL(triggered()), SLOT(ui_totalSize()));

    QAction *shardVersion = new QAction("Shard Version", this);
    connect(shardVersion, SIGNAL(triggered()), SLOT(ui_shardVersion()));

    QAction *shardDistribution = new QAction("Shard Distribution", this);
    connect(shardDistribution, SIGNAL(triggered()), SLOT(ui_shardDistribution()));

    QAction *dropCollection = new QAction("Drop Collection", this);
    connect(dropCollection, SIGNAL(triggered()), SLOT(ui_dropCollection()));

    QAction *renameCollection = new QAction("Rename Collection", this);
    connect(renameCollection, SIGNAL(triggered()), SLOT(ui_renameCollection()));

    QAction *duplicateCollection = new QAction("Duplicate Collection", this);
    connect(duplicateCollection, SIGNAL(triggered()), SLOT(ui_duplicateCollection()));

    QAction *viewCollection = new QAction("View Documents", this);
    connect(viewCollection, SIGNAL(triggered()), SLOT(ui_viewCollection()));

    _collectionContextMenu = new QMenu(this);
    _collectionContextMenu->addAction(viewCollection);
    _collectionContextMenu->addSeparator();
    _collectionContextMenu->addAction(addDocument);
    _collectionContextMenu->addAction(updateDocument);
    _collectionContextMenu->addAction(removeDocument);
    _collectionContextMenu->addAction(removeAllDocuments);
    _collectionContextMenu->addSeparator();
    _collectionContextMenu->addAction(renameCollection);
    _collectionContextMenu->addAction(duplicateCollection);
    _collectionContextMenu->addAction(dropCollection);
    _collectionContextMenu->addSeparator();
    _collectionContextMenu->addAction(addIndex);
    _collectionContextMenu->addAction(dropIndex);
    _collectionContextMenu->addAction(reIndex);
    _collectionContextMenu->addSeparator();
    _collectionContextMenu->addAction(collectionStats);
    _collectionContextMenu->addSeparator();
    _collectionContextMenu->addAction(shardVersion);
    _collectionContextMenu->addAction(shardDistribution);


    QAction *dropUser = new QAction("Remove User", this);
    connect(dropUser, SIGNAL(triggered()), SLOT(ui_dropUser()));

    QAction *editUser = new QAction("Edit User", this);
    connect(editUser, SIGNAL(triggered()), SLOT(ui_editUser()));

    _userContextMenu = new QMenu(this);
    _userContextMenu->addAction(editUser);
    _userContextMenu->addAction(dropUser);


    QAction *createCollection = new QAction("Create Collection", this);
    connect(createCollection, SIGNAL(triggered()), SLOT(ui_createCollection()));

    QAction *dbCollectionsStats = new QAction("Collections Statistics", this);
    connect(dbCollectionsStats, SIGNAL(triggered()), SLOT(ui_dbCollectionsStatistics()));

    QAction *refreshCollections = new QAction("Refresh", this);
    connect(refreshCollections, SIGNAL(triggered()), SLOT(ui_refreshCollections()));

    _collectionCategoryContextMenu = new QMenu(this);
    _collectionCategoryContextMenu->addAction(dbCollectionsStats);
    _collectionCategoryContextMenu->addAction(createCollection);
    _collectionCategoryContextMenu->addSeparator();
    _collectionCategoryContextMenu->addAction(refreshCollections);


    QAction *refreshUsers = new QAction("Refresh", this);
    connect(refreshUsers, SIGNAL(triggered()), SLOT(ui_refreshUsers()));

    QAction *viewUsers = new QAction("View Users", this);
    connect(viewUsers, SIGNAL(triggered()), SLOT(ui_viewUsers()));

    QAction *addUser = new QAction("Add User", this);
    connect(addUser, SIGNAL(triggered()), SLOT(ui_addUser()));

    _usersCategoryContextMenu = new QMenu(this);
    _usersCategoryContextMenu->addAction(viewUsers);
    _usersCategoryContextMenu->addAction(addUser);
    _usersCategoryContextMenu->addSeparator();
    _usersCategoryContextMenu->addAction(refreshUsers);


    QAction *dropFunction = new QAction("Remove Function", this);
    connect(dropFunction, SIGNAL(triggered()), SLOT(ui_dropFunction()));

    QAction *editFunction = new QAction("Edit Function", this);
    connect(editFunction, SIGNAL(triggered()), SLOT(ui_editFunction()));

    _functionContextMenu = new QMenu(this);
    _functionContextMenu->addAction(editFunction);
    _functionContextMenu->addAction(dropFunction);


    QAction *refreshFunctions = new QAction("Refresh", this);
    connect(refreshFunctions, SIGNAL(triggered()), SLOT(ui_refreshFunctions()));

    QAction *viewFunctions = new QAction("View Functions", this);
    connect(viewFunctions, SIGNAL(triggered()), SLOT(ui_viewFunctions()));

    QAction *addFunction = new QAction("Add Function", this);
    connect(addFunction, SIGNAL(triggered()), SLOT(ui_addFunction()));

    _functionsCategoryContextMenu = new QMenu(this);
    _functionsCategoryContextMenu->addAction(viewFunctions);
    _functionsCategoryContextMenu->addAction(addFunction);
    _functionsCategoryContextMenu->addSeparator();
    _functionsCategoryContextMenu->addAction(refreshFunctions);
}

void ExplorerTreeWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QTreeWidgetItem *item = itemAt(event->pos());
    if (!item)
        return;

    ExplorerServerTreeItem *serverItem = dynamic_cast<ExplorerServerTreeItem *>(item);
    if (serverItem) {
        _serverContextMenu->exec(mapToGlobal(event->pos()));
        return;
    }

    ExplorerCollectionTreeItem *collectionItem = dynamic_cast<ExplorerCollectionTreeItem *>(item);
    if (collectionItem) {
        _collectionContextMenu->exec(mapToGlobal(event->pos()));
        return;
    }

    ExplorerDatabaseTreeItem *databaseItem = dynamic_cast<ExplorerDatabaseTreeItem *>(item);
    if (databaseItem) {
        _databaseContextMenu->exec(mapToGlobal(event->pos()));
        return;
    }

    ExplorerUserTreeItem *userItem = dynamic_cast<ExplorerUserTreeItem *>(item);
    if (userItem) {
        _userContextMenu->exec(mapToGlobal(event->pos()));
        return;
    }

    ExplorerFunctionTreeItem *functionItem = dynamic_cast<ExplorerFunctionTreeItem *>(item);
    if (functionItem) {
        _functionContextMenu->exec(mapToGlobal(event->pos()));
        return;
    }

    ExplorerDatabaseCategoryTreeItem *categoryItem = dynamic_cast<ExplorerDatabaseCategoryTreeItem *>(item);
    if (categoryItem) {
        if (categoryItem->category() == Collections) {
            _collectionCategoryContextMenu->exec(mapToGlobal(event->pos()));
        } else if (categoryItem->category() == Users) {
            _usersCategoryContextMenu->exec(mapToGlobal(event->pos()));
        } else if (categoryItem->category() == Functions) {
            _functionsCategoryContextMenu->exec(mapToGlobal(event->pos()));
        }
        return;
    }
}

ExplorerServerTreeItem *ExplorerTreeWidget::selectedServerItem()
{
    QList<QTreeWidgetItem*> items = selectedItems();

    if (items.count() != 1)
        return NULL;

    QTreeWidgetItem *item = items[0];

    if (!item)
        return NULL;

    ExplorerServerTreeItem *serverItem = dynamic_cast<ExplorerServerTreeItem *>(item);
    if (!serverItem)
        return NULL;

    return serverItem;
}

ExplorerCollectionTreeItem *ExplorerTreeWidget::selectedCollectionItem()
{
    QList<QTreeWidgetItem*> items = selectedItems();

    if (items.count() != 1)
        return NULL;

    QTreeWidgetItem *item = items[0];

    if (!item)
        return NULL;

    ExplorerCollectionTreeItem *collectionItem = dynamic_cast<ExplorerCollectionTreeItem *>(item);
    if (!collectionItem)
        return NULL;

    return collectionItem;
}

ExplorerUserTreeItem *ExplorerTreeWidget::selectedUserItem()
{
    QList<QTreeWidgetItem*> items = selectedItems();

    if (items.count() != 1)
        return NULL;

    QTreeWidgetItem *item = items[0];

    if (!item)
        return NULL;

    ExplorerUserTreeItem *userItem = dynamic_cast<ExplorerUserTreeItem *>(item);
    if (!userItem)
        return NULL;

    return userItem;
}

ExplorerFunctionTreeItem *ExplorerTreeWidget::selectedFunctionItem()
{
    QList<QTreeWidgetItem*> items = selectedItems();

    if (items.count() != 1)
        return NULL;

    QTreeWidgetItem *item = items[0];

    if (!item)
        return NULL;

    ExplorerFunctionTreeItem *funItem = dynamic_cast<ExplorerFunctionTreeItem *>(item);
    if (!funItem)
        return NULL;

    return funItem;
}

ExplorerDatabaseTreeItem *ExplorerTreeWidget::selectedDatabaseItem()
{
    QList<QTreeWidgetItem*> items = selectedItems();

    if (items.count() != 1)
        return NULL;

    QTreeWidgetItem *item = items[0];

    if (!item)
        return NULL;

    ExplorerDatabaseTreeItem *dbtem = dynamic_cast<ExplorerDatabaseTreeItem *>(item);
    if (!dbtem)
        return NULL;

    return dbtem;
}

ExplorerDatabaseCategoryTreeItem *ExplorerTreeWidget::selectedDatabaseCategoryItem()
{
    QList<QTreeWidgetItem*> items = selectedItems();

    if (items.count() != 1)
        return NULL;

    QTreeWidgetItem *item = items[0];

    if (!item)
        return NULL;

    ExplorerDatabaseCategoryTreeItem *categoryItem = dynamic_cast<ExplorerDatabaseCategoryTreeItem *>(item);
    if (!categoryItem)
        return NULL;

    return categoryItem;
}

void ExplorerTreeWidget::openCurrentCollectionShell(const QString &script, bool execute,
                                                    const CursorPosition &cursor)
{
    ExplorerCollectionTreeItem *collectionItem = selectedCollectionItem();
    if (!collectionItem)
        return;

    MongoCollection *collection = collectionItem->collection();
    App *app = AppRegistry::instance().app();
    QString query = app->buildCollectionQuery(collection->name(), script);

    AppRegistry::instance().app()->
        openShell(collection->database(), query, execute, collection->name(), cursor);
}

void ExplorerTreeWidget::openCurrentDatabaseShell(const QString &script, bool execute,
                                                  const CursorPosition &cursor)
{
    ExplorerDatabaseTreeItem *collectionItem = selectedDatabaseItem();
    if (!collectionItem)
        return;

    MongoDatabase *database = collectionItem->database();
    openDatabaseShell(database, script, execute, cursor);
}

void ExplorerTreeWidget::openCurrentServerShell(const QString &script, bool execute,
                                                const CursorPosition &cursor)
{
    ExplorerServerTreeItem *serverItem = selectedServerItem();
    if (!serverItem)
        return;

    MongoServer *server = serverItem->server();

    AppRegistry::instance().app()->
            openShell(server, script, QString(), execute, server->connectionRecord()->getReadableName(), cursor);
}

void ExplorerTreeWidget::openDatabaseShell(MongoDatabase *database, const QString &script, bool execute, const CursorPosition &cursor)
{
    AppRegistry::instance().app()->
        openShell(database, script, execute, database->name(), cursor);
}

void ExplorerTreeWidget::ui_disconnectServer()
{
    emit disconnectActionTriggered();
}

void ExplorerTreeWidget::ui_refreshServer()
{
    ExplorerServerTreeItem *serverItem = selectedServerItem();
    if (!serverItem)
        return;

    serverItem->expand();
}

void ExplorerTreeWidget::ui_createDatabase()
{
    ExplorerServerTreeItem *serverItem = selectedServerItem();
    if (!serverItem)
        return;

    CreateDatabaseDialog dlg(serverItem->server()->connectionRecord()->getFullAddress());
    dlg.setOkButtonText("&Create");
    dlg.setInputLabelText("Database Name:");
    int result = dlg.exec();

    if (result == QDialog::Accepted) {
        serverItem->server()->createDatabase(dlg.databaseName());

        // refresh list of databases
        serverItem->expand();
    }
}

void ExplorerTreeWidget::ui_showLog()
{
    openCurrentServerShell("show log");
}

void ExplorerTreeWidget::ui_openShell()
{
    emit openShellActionTriggered();
}

void ExplorerTreeWidget::ui_addDocument()
{
    ExplorerCollectionTreeItem *collectionItem = selectedCollectionItem();
    if (!collectionItem)
        return;

    MongoCollection *collection = collectionItem->collection();
    MongoDatabase *database = collection->database();
    MongoServer *server = database->server();
    ConnectionSettings *settings = server->connectionRecord();

    DocumentTextEditor editor(settings->getFullAddress(), database->name(),
                              collection->name(), "{\n    \n}");

    editor.setCursorPosition(1, 4);
    editor.setWindowTitle("Insert Document");
    int result = editor.exec();
    activateWindow();

    if (result == QDialog::Accepted) {
        mongo::BSONObj obj = editor.bsonObj();
        server->insertDocument(obj, database->name(), collection->name());
    }

    /*
    openCurrentCollectionShell(
        "insert({\n"
        "    '' : '',\n"
        "})"
    , false, CursorPosition(1, 5));
    */
}

void ExplorerTreeWidget::ui_removeDocument()
{
    openCurrentCollectionShell(
        "remove({ '' : '' });"
        , false, CursorPosition(0, -10));
}

void ExplorerTreeWidget::ui_removeAllDocuments()
{
    ExplorerCollectionTreeItem *collectionItem = selectedCollectionItem();
    if (!collectionItem)
        return;

    MongoCollection *collection = collectionItem->collection();
    MongoDatabase *database = collection->database();
    MongoServer *server = database->server();
    ConnectionSettings *settings = server->connectionRecord();

    // Ask user
    int answer = QMessageBox::question(this,
            "Remove All Documents",
            QString("Remove all documents from <b>%1</b> collection?").arg(collection->name()),
            QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

    if (answer != QMessageBox::Yes)
        return;

    mongo::BSONObjBuilder builder;
    mongo::BSONObj bsonQuery = builder.obj();
    mongo::Query query(bsonQuery);

    server->removeDocuments(query, database->name(), collection->name(), false);
}

void ExplorerTreeWidget::ui_addIndex()
{
    openCurrentCollectionShell(
        "ensureIndex({ \"<field>\" : 1 }); \n"
        "\n"
        "// options: \n"
        "// { unique : true }   - A unique index causes MongoDB to reject all documents that contain a duplicate value for the indexed field. \n"
        "// { sparse : true }   - Sparse indexes only contain entries for documents that have the indexed field. \n"
        "// { dropDups : true } - Sparse indexes only contain entries for documents that have the indexed field. \n"
    , false);
}

void ExplorerTreeWidget::ui_reIndex()
{
    openCurrentCollectionShell("reIndex()", false);
}

void ExplorerTreeWidget::ui_dropIndex()
{
    openCurrentCollectionShell(
        "dropIndex({ \"<field>\" : 1 });"
        , false);
}

void ExplorerTreeWidget::ui_updateDocument()
{
    openCurrentCollectionShell(
        "update(\n"
        "    // query \n"
        "    {\n"
        "        \"key\" : \"value\"\n"
        "    },\n"
        "    \n"
        "    // update \n"
        "    {\n"
        "    },\n"
        "    \n"
        "    // options \n"
        "    {\n"
        "        \"multi\" : false,  // update only one document \n"
        "        \"upsert\" : false  // insert a new document, if no existing document match the query \n"
        "    }\n"
        ");", false);
}

void ExplorerTreeWidget::ui_collectionStatistics()
{
    openCurrentCollectionShell("stats()");
}

void ExplorerTreeWidget::ui_dropCollection()
{
    ExplorerCollectionTreeItem *collectionItem = selectedCollectionItem();
    if (!collectionItem)
        return;

    MongoCollection *collection = collectionItem->collection();
    MongoDatabase *database = collection->database();
    MongoServer *server = database->server();
    ConnectionSettings *settings = server->connectionRecord();

    // Ask user
    int answer = QMessageBox::question(this,
            "Drop Collection",
            QString("Drop <b>%1</b> collection?").arg(collection->name()),
            QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

    if (answer != QMessageBox::Yes)
        return;

    database->dropCollection(collection->name());
    database->loadCollections();

    //openCurrentCollectionShell("drop()", false);
}

void ExplorerTreeWidget::ui_duplicateCollection()
{
    ExplorerCollectionTreeItem *collectionItem = selectedCollectionItem();
    if (!collectionItem)
        return;

    MongoCollection *collection = collectionItem->collection();
    MongoDatabase *database = collection->database();
    MongoServer *server = database->server();
    ConnectionSettings *settings = server->connectionRecord();

    CreateDatabaseDialog dlg(settings->getFullAddress(),
                             database->name(),
                             collection->name());
    dlg.setWindowTitle("Duplicate Collection");
    dlg.setOkButtonText("&Duplicate");
    dlg.setInputLabelText("New Collection Name:");
    dlg.setInputText(collection->name() + "_copy");
    int result = dlg.exec();

    if (result == QDialog::Accepted) {
        database->duplicateCollection(collection->name(), dlg.databaseName());

        // refresh list of collections
        database->loadCollections();
    }
}

void ExplorerTreeWidget::ui_renameCollection()
{
    ExplorerCollectionTreeItem *collectionItem = selectedCollectionItem();
    if (!collectionItem)
        return;

    MongoCollection *collection = collectionItem->collection();
    MongoDatabase *database = collection->database();
    MongoServer *server = database->server();
    ConnectionSettings *settings = server->connectionRecord();

    CreateDatabaseDialog dlg(settings->getFullAddress(),
                             database->name(),
                             collection->name());
    dlg.setWindowTitle("Rename Collection");
    dlg.setOkButtonText("&Rename");
    dlg.setInputLabelText("New Collection Name:");
    dlg.setInputText(collection->name());
    int result = dlg.exec();

    if (result == QDialog::Accepted) {
        database->renameCollection(collection->name(), dlg.databaseName());

        // refresh list of collections
        database->loadCollections();
    }
}

void ExplorerTreeWidget::ui_viewCollection()
{
    openCurrentCollectionShell("find()");
}

void ExplorerTreeWidget::ui_storageSize()
{
    openCurrentCollectionShell("storageSize()");
}

void ExplorerTreeWidget::ui_totalIndexSize()
{
    openCurrentCollectionShell("totalIndexSize()");
}

void ExplorerTreeWidget::ui_totalSize()
{
    openCurrentCollectionShell("totalSize()");
}

void ExplorerTreeWidget::ui_shardVersion()
{
    openCurrentCollectionShell("getShardVersion()");
}

void ExplorerTreeWidget::ui_shardDistribution()
{
    openCurrentCollectionShell("getShardDistribution()");
}

void ExplorerTreeWidget::ui_dbStatistics()
{
    openCurrentDatabaseShell("db.stats()");
}

void ExplorerTreeWidget::ui_dbDrop()
{
    ExplorerDatabaseTreeItem *dbItem = selectedDatabaseItem();
    if (!dbItem)
        return;

    MongoDatabase *database = dbItem->database();
    MongoServer *server = database->server();

    // Ask user
    int answer = QMessageBox::question(this,
            "Drop Database",
            QString("Drop <b>%1</b> database?").arg(database->name()),
            QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

    if (answer != QMessageBox::Yes)
        return;

    server->dropDatabase(database->name());
    server->loadDatabases(); // refresh list of databases

    //openCurrentDatabaseShell("db.dropDatabase()", false);
}

void ExplorerTreeWidget::ui_dbCollectionsStatistics()
{
    ExplorerDatabaseCategoryTreeItem *categoryItem = selectedDatabaseCategoryItem();
    if (!categoryItem)
        return;

    openDatabaseShell(categoryItem->databaseItem()->database(), "db.printCollectionStats()");
}

void ExplorerTreeWidget::ui_dbRepair()
{
    openCurrentDatabaseShell("db.repairDatabase()", false);
}

void ExplorerTreeWidget::ui_dbOpenShell()
{
    openCurrentDatabaseShell("");
}

void ExplorerTreeWidget::ui_serverHostInfo()
{
    openCurrentServerShell("db.hostInfo()");
}

void ExplorerTreeWidget::ui_serverStatus()
{
    openCurrentServerShell("db.serverStatus()");
}

void ExplorerTreeWidget::ui_serverVersion()
{
    openCurrentServerShell("db.version()");
}

void ExplorerTreeWidget::ui_refreshUsers()
{
    ExplorerDatabaseCategoryTreeItem *categoryItem = selectedDatabaseCategoryItem();
    if (!categoryItem)
        return;

    categoryItem->databaseItem()->expandUsers();
}

void ExplorerTreeWidget::ui_refreshFunctions()
{
    ExplorerDatabaseCategoryTreeItem *categoryItem = selectedDatabaseCategoryItem();
    if (!categoryItem)
        return;

    categoryItem->databaseItem()->expandFunctions();
}

void ExplorerTreeWidget::ui_viewUsers()
{
    ExplorerDatabaseCategoryTreeItem *categoryItem = selectedDatabaseCategoryItem();
    if (!categoryItem)
        return;

    openDatabaseShell(categoryItem->databaseItem()->database(),
                      "db.system.users.find()");
}

void ExplorerTreeWidget::ui_viewFunctions()
{
    ExplorerDatabaseCategoryTreeItem *categoryItem = selectedDatabaseCategoryItem();
    if (!categoryItem)
        return;

    openDatabaseShell(categoryItem->databaseItem()->database(),
                      "db.system.js.find()");
}

void ExplorerTreeWidget::ui_refreshDatabase()
{
    ExplorerDatabaseTreeItem *databaseItem = selectedDatabaseItem();
    if (!databaseItem)
        return;

    databaseItem->expandCollections();
}

void ExplorerTreeWidget::ui_createCollection()
{
    ExplorerDatabaseCategoryTreeItem *categoryItem = selectedDatabaseCategoryItem();
    if (!categoryItem)
        return;

    ExplorerDatabaseTreeItem *databaseItem = categoryItem->databaseItem();
    if (!databaseItem)
        return;

    CreateDatabaseDialog dlg(databaseItem->database()->server()->connectionRecord()->getFullAddress(),
                             databaseItem->database()->name());
    dlg.setWindowTitle("Create Collection");
    dlg.setOkButtonText("&Create");
    dlg.setInputLabelText("Collection Name:");
    int result = dlg.exec();

    if (result == QDialog::Accepted) {
        databaseItem->database()->createCollection(dlg.databaseName());

        // refresh list of databases
        databaseItem->expandCollections();
    }
}

void ExplorerTreeWidget::ui_addUser()
{
    ExplorerDatabaseCategoryTreeItem *categoryItem = selectedDatabaseCategoryItem();
    if (!categoryItem)
        return;

    ExplorerDatabaseTreeItem *databaseItem = categoryItem->databaseItem();

    CreateUserDialog dlg(databaseItem->database()->server()->connectionRecord()->getFullAddress(),
                         databaseItem->database()->name());
    int result = dlg.exec();

    if (result == QDialog::Accepted) {

        MongoUser user = dlg.user();
        databaseItem->database()->createUser(user, false);

        // refresh list of users
        databaseItem->expandUsers();
    }
}

void ExplorerTreeWidget::ui_addFunction()
{
    ExplorerDatabaseCategoryTreeItem *categoryItem = selectedDatabaseCategoryItem();
    if (!categoryItem)
        return;

    ExplorerDatabaseTreeItem *databaseItem = categoryItem->databaseItem();

    FunctionTextEditor dlg(databaseItem->database()->server()->connectionRecord()->getFullAddress(),
                           databaseItem->database()->name(), MongoFunction());
    dlg.setWindowTitle("Create Function");
    dlg.setCode(
        "function() {\n"
        "    // write your code here\n"
        "}");
    dlg.setCursorPosition(1, 4);

    int result = dlg.exec();

    if (result == QDialog::Accepted) {

        MongoFunction function = dlg.function();
        databaseItem->database()->createFunction(function);

        // refresh list of functions
        databaseItem->expandFunctions();
    }
}

void ExplorerTreeWidget::ui_editFunction()
{
    ExplorerFunctionTreeItem *functionItem = selectedFunctionItem();
    if (!functionItem)
        return;

    MongoFunction function = functionItem->function();
    MongoDatabase *database = functionItem->database();
    MongoServer *server = database->server();
    QString name = function.name();

    FunctionTextEditor dlg(server->connectionRecord()->getFullAddress(),
                         database->name(),
                         function);
    dlg.setWindowTitle("Edit Function");
    int result = dlg.exec();

    if (result == QDialog::Accepted) {

        MongoFunction editedFunction = dlg.function();
        database->updateFunction(name, editedFunction);

        // refresh list of functions
        database->loadFunctions();
    }
}

void ExplorerTreeWidget::ui_dropUser()
{
    ExplorerUserTreeItem *userItem = selectedUserItem();
    if (!userItem)
        return;

    MongoUser user = userItem->user();
    MongoDatabase *database = userItem->database();
    MongoServer *server = database->server();

    // Ask user
    int answer = QMessageBox::question(this,
            "Remove User",
            QString("Remove <b>%1</b> user?").arg(user.name()),
            QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

    if (answer != QMessageBox::Yes)
        return;

    database->dropUser(user.id());
    database->loadUsers(); // refresh list of users
}

void ExplorerTreeWidget::ui_dropFunction()
{
    ExplorerFunctionTreeItem *functionItem = selectedFunctionItem();
    if (!functionItem)
        return;

    MongoFunction function = functionItem->function();
    MongoDatabase *database = functionItem->database();

    // Ask user
    int answer = QMessageBox::question(this,
            "Remove Function",
            QString("Remove <b>%1</b> function?").arg(function.name()),
            QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

    if (answer != QMessageBox::Yes)
        return;

    database->dropFunction(function.name());
    database->loadFunctions(); // refresh list of functions
}

void ExplorerTreeWidget::ui_editUser()
{
    ExplorerUserTreeItem *userItem = selectedUserItem();
    if (!userItem)
        return;

    MongoUser user = userItem->user();
    MongoDatabase *database = userItem->database();
    MongoServer *server = database->server();

    CreateUserDialog dlg(server->connectionRecord()->getFullAddress(),
                         database->name(),
                         user);
    dlg.setWindowTitle("Edit User");
    dlg.setUserPasswordLabelText("New Password:");
    int result = dlg.exec();

    if (result == QDialog::Accepted) {

        MongoUser user = dlg.user();
        database->createUser(user, true);

        // refresh list of users
        database->loadUsers();
    }
}

void ExplorerTreeWidget::ui_refreshCollections()
{
    ExplorerDatabaseCategoryTreeItem *categoryItem = selectedDatabaseCategoryItem();
    if (!categoryItem)
        return;

    categoryItem->databaseItem()->expandCollections();
}
