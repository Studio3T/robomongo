#include "robomongo/gui/widgets/explorer/ExplorerTreeWidget.h"

#include <QContextMenuEvent>
#include <QMenu>
#include <QtGui>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/widgets/explorer/ExplorerServerTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseCategoryTreeItem.h"
#include "robomongo/gui/dialogs/DocumentTextEditor.h"

#include "robomongo/shell/db/json.h"

using namespace Robomongo;

ExplorerTreeWidget::ExplorerTreeWidget(QWidget *parent) : QTreeWidget(parent)
{
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

    QAction *refreshServer = new QAction("Refresh", this);
    connect(refreshServer, SIGNAL(triggered()), SLOT(ui_refreshServer()));

    _serverContextMenu = new QMenu(this);
    _serverContextMenu->addAction(openShellAction);
    _serverContextMenu->addAction(refreshServer);
    _serverContextMenu->addSeparator();
    _serverContextMenu->addAction(serverStatus);
    _serverContextMenu->addAction(serverHostInfo);
    _serverContextMenu->addAction(serverVersion);
    _serverContextMenu->addSeparator();
    _serverContextMenu->addAction(disconnectAction);

    QAction *openDbShellAction = new QAction("Open Shell", this);
    openDbShellAction->setIcon(GuiRegistry::instance().mongodbIcon());
    connect(openDbShellAction, SIGNAL(triggered()), SLOT(ui_dbOpenShell()));

    QAction *dbStats = new QAction("Statistics", this);
    connect(dbStats, SIGNAL(triggered()), SLOT(ui_dbStatistics()));

    QAction *dbDrop = new QAction("Drop Database", this);
    connect(dbDrop, SIGNAL(triggered()), SLOT(ui_dbDrop()));

    QAction *dbCollectionsStats = new QAction("Collections Statistics", this);
    connect(dbCollectionsStats, SIGNAL(triggered()), SLOT(ui_dbCollectionsStatistics()));

    QAction *dbRepair = new QAction("Repair", this);
    connect(dbRepair, SIGNAL(triggered()), SLOT(ui_dbRepair()));

    QAction *refreshDatabase = new QAction("Refresh", this);
    connect(refreshDatabase, SIGNAL(triggered()), SLOT(ui_refreshDatabase()));

    _databaseContextMenu = new QMenu(this);
    _databaseContextMenu->addAction(openDbShellAction);
    _databaseContextMenu->addAction(refreshDatabase);
    _databaseContextMenu->addAction(dbRepair);
    _databaseContextMenu->addSeparator();
    _databaseContextMenu->addAction(dbStats);
    _databaseContextMenu->addAction(dbCollectionsStats);
    _databaseContextMenu->addSeparator();
    _databaseContextMenu->addAction(dbDrop);

    QAction *addDocument = new QAction("Insert Document", this);
    connect(addDocument, SIGNAL(triggered()), SLOT(ui_addDocument()));

    QAction *updateDocument = new QAction("Update Document", this);
    connect(updateDocument, SIGNAL(triggered()), SLOT(ui_updateDocument()));

    QAction *removeDocument = new QAction("Remove Document", this);
    connect(removeDocument, SIGNAL(triggered()), SLOT(ui_removeDocument()));

    QAction *addIndex = new QAction("Add Index", this);
    connect(addIndex, SIGNAL(triggered()), SLOT(ui_addIndex()));

    QAction *dropIndex = new QAction("Drop Index", this);
    connect(dropIndex, SIGNAL(triggered()), SLOT(ui_dropIndex()));

    QAction *reIndex = new QAction("Reindex", this);
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

    _collectionContextMenu = new QMenu(this);
    _collectionContextMenu->addAction(addDocument);
    _collectionContextMenu->addAction(updateDocument);
    _collectionContextMenu->addAction(removeDocument);
    _collectionContextMenu->addSeparator();
    _collectionContextMenu->addAction(addIndex);
    _collectionContextMenu->addAction(dropIndex);
    _collectionContextMenu->addAction(reIndex);
    _collectionContextMenu->addSeparator();
    _collectionContextMenu->addAction(collectionStats);
    _collectionContextMenu->addAction(dropCollection);
    _collectionContextMenu->addSeparator();
    _collectionContextMenu->addAction(shardVersion);
    _collectionContextMenu->addAction(shardDistribution);


    QAction *refreshCollections = new QAction("Refresh", this);
    connect(refreshCollections, SIGNAL(triggered()), SLOT(ui_refreshCollections()));

    _collectionCategoryContextMenu = new QMenu(this);
    _collectionCategoryContextMenu->addAction(refreshCollections);
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

    ExplorerDatabaseCategoryTreeItem *collectionCategoryItem = dynamic_cast<ExplorerDatabaseCategoryTreeItem *>(item);
    if (collectionCategoryItem) {
        if (collectionCategoryItem->category() == Collections) {
            _collectionCategoryContextMenu->exec(mapToGlobal(event->pos()));
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
    QString query = QString(script).arg(collection->name());

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

    AppRegistry::instance().app()->
        openShell(database, script, execute, database->name(), cursor);
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
        "db.%1.insert({\n"
        "    '' : '',\n"
        "})"
    , false, CursorPosition(1, 5));
    */
}

void ExplorerTreeWidget::ui_removeDocument()
{
    openCurrentCollectionShell(
        "db.%1.remove({ '' : '' });"
    , false, CursorPosition(0, -10));
}

void ExplorerTreeWidget::ui_addIndex()
{
    openCurrentCollectionShell(
        "db.%1.ensureIndex({ \"<field>\" : 1 }); \n"
        "\n"
        "// options: \n"
        "// { unique : true }   - A unique index causes MongoDB to reject all documents that contain a duplicate value for the indexed field. \n"
        "// { sparse : true }   - Sparse indexes only contain entries for documents that have the indexed field. \n"
        "// { dropDups : true } - Sparse indexes only contain entries for documents that have the indexed field. \n"
    , false);
}

void ExplorerTreeWidget::ui_reIndex()
{
    openCurrentCollectionShell("db.%1.reIndex()");
}

void ExplorerTreeWidget::ui_dropIndex()
{
    openCurrentCollectionShell(
        "db.%1.dropIndex({ \"<field>\" : 1 });"
        , false);
}

void ExplorerTreeWidget::ui_updateDocument()
{
    openCurrentCollectionShell(
        "db.%1.update(\n"
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
        "});", false);
}

void ExplorerTreeWidget::ui_collectionStatistics()
{
    openCurrentCollectionShell("db.%1.stats()");
}

void ExplorerTreeWidget::ui_dropCollection()
{
    openCurrentCollectionShell("db.%1.drop()", false);
}

void ExplorerTreeWidget::ui_storageSize()
{
    openCurrentCollectionShell("db.%1.storageSize()");
}

void ExplorerTreeWidget::ui_totalIndexSize()
{
    openCurrentCollectionShell("db.%1.totalIndexSize()");
}

void ExplorerTreeWidget::ui_totalSize()
{
    openCurrentCollectionShell("db.%1.totalSize()");
}

void ExplorerTreeWidget::ui_shardVersion()
{
    openCurrentCollectionShell("db.%1.getShardVersion()");
}

void ExplorerTreeWidget::ui_shardDistribution()
{
    openCurrentCollectionShell("db.%1.getShardDistribution()");
}

void ExplorerTreeWidget::ui_dbStatistics()
{
    openCurrentDatabaseShell("db.stats()");
}

void ExplorerTreeWidget::ui_dbDrop()
{
    openCurrentDatabaseShell("db.dropDatabase()", false);
}

void ExplorerTreeWidget::ui_dbCollectionsStatistics()
{
    openCurrentDatabaseShell("db.printCollectionStats()");
}

void ExplorerTreeWidget::ui_dbRepair()
{
    openCurrentDatabaseShell("db.repairDatabase()");
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

void ExplorerTreeWidget::ui_refreshDatabase()
{
    ExplorerDatabaseTreeItem *databaseItem = selectedDatabaseItem();
    if (!databaseItem)
        return;

    databaseItem->expandCollections();
}

void ExplorerTreeWidget::ui_refreshCollections()
{
    ExplorerDatabaseCategoryTreeItem *categoryItem = selectedDatabaseCategoryItem();
    if (!categoryItem)
        return;

    categoryItem->databaseItem()->expandCollections();
}
