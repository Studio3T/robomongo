#include "ExplorerTreeWidget.h"
#include <QContextMenuEvent>
#include "AppRegistry.h"
//#include "ExplorerServerTreeItem.h"
#include <QMenu>
#include <QtGui>
#include "GuiRegistry.h"
#include "ExplorerServerTreeItem.h"
#include "ExplorerCollectionTreeItem.h"
#include "domain/MongoCollection.h"
#include "domain/MongoServer.h"
#include "domain/App.h"
#include "settings/ConnectionRecord.h"
#include "AppRegistry.h"
#include "ExplorerDatabaseTreeItem.h"

using namespace Robomongo;

ExplorerTreeWidget::ExplorerTreeWidget(QWidget *parent) : QTreeWidget(parent)
{
    //setVerticalScrollMode()
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setObjectName("explorerTree");

    // Connect action
    QAction *disconnectAction = new QAction("Disconnect", this);
    //disconnectAction->setIcon(GuiRegistry::instance().serverIcon());
    disconnectAction->setIconText("Disconnect");
    connect(disconnectAction, SIGNAL(triggered()), SLOT(ui_disconnectServer()));

    // Refresh action
    QAction *refreshAction = new QAction("Refresh", this);
    refreshAction->setIcon(qApp->style()->standardIcon(QStyle::SP_BrowserReload));
    connect(refreshAction, SIGNAL(triggered()), SLOT(ui_refreshServer()));

    // Open shell
    QAction *openShellAction = new QAction("Open Shell", this);
    openShellAction->setIcon(GuiRegistry::instance().mongodbIcon());
    connect(openShellAction, SIGNAL(triggered()), SLOT(ui_openShell()));

    // Open shell
    QAction *serverHostInfo = new QAction("Host Info", this);
    connect(serverHostInfo, SIGNAL(triggered()), SLOT(ui_serverHostInfo()));

    QAction *serverStatus = new QAction("Server Status", this);
    connect(serverStatus, SIGNAL(triggered()), SLOT(ui_serverStatus()));

    QAction *serverVersion = new QAction("MongoDB Version", this);
    connect(serverVersion, SIGNAL(triggered()), SLOT(ui_serverVersion()));

    // File menu
    _serverMenu = new QMenu(this);
    _serverMenu->addAction(openShellAction);
    _serverMenu->addSeparator();
    _serverMenu->addAction(serverStatus);
    _serverMenu->addAction(serverHostInfo);
    _serverMenu->addAction(serverVersion);
    _serverMenu->addSeparator();
    _serverMenu->addAction(disconnectAction);

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

    _databaseMenu = new QMenu(this);
    _databaseMenu->addAction(openDbShellAction);
    _databaseMenu->addAction(dbRepair);
    _databaseMenu->addSeparator();
    _databaseMenu->addAction(dbStats);
    _databaseMenu->addAction(dbCollectionsStats);
    _databaseMenu->addSeparator();
    _databaseMenu->addAction(dbDrop);

    QAction *addDocument = new QAction("Add Document", this);
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

    _collectionMenu = new QMenu(this);
    _collectionMenu->addAction(addDocument);
    _collectionMenu->addAction(updateDocument);
    _collectionMenu->addAction(removeDocument);
    _collectionMenu->addSeparator();
    _collectionMenu->addAction(addIndex);
    _collectionMenu->addAction(dropIndex);
    _collectionMenu->addAction(reIndex);
    _collectionMenu->addSeparator();
    _collectionMenu->addAction(collectionStats);
    _collectionMenu->addSeparator();
    _collectionMenu->addAction(shardVersion);
    _collectionMenu->addAction(shardDistribution);
}

ExplorerTreeWidget::~ExplorerTreeWidget()
{
    int a = 67;
}

void ExplorerTreeWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QTreeWidgetItem *item = itemAt(event->pos());
    if (!item)
        return;

    ExplorerServerTreeItem *serverItem = dynamic_cast<ExplorerServerTreeItem *>(item);
    if (serverItem) {
        _serverMenu->exec(mapToGlobal(event->pos()));
        return;
    }

    ExplorerCollectionTreeItem *collectionItem = dynamic_cast<ExplorerCollectionTreeItem *>(item);
    if (collectionItem) {
        _collectionMenu->exec(mapToGlobal(event->pos()));
        return;
    }

    ExplorerDatabaseTreeItem *databaseItem = dynamic_cast<ExplorerDatabaseTreeItem *>(item);
    if (databaseItem) {
        _databaseMenu->exec(mapToGlobal(event->pos()));
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

void ExplorerTreeWidget::openCurrentCollectionShell(const QString &script, bool execute)
{
    ExplorerCollectionTreeItem *collectionItem = selectedCollectionItem();
    if (!collectionItem)
        return;

    MongoCollection *collection = collectionItem->collection();
    QString query = QString(script).arg(collection->name());

    AppRegistry::instance().app()->
            openShell(collection->database(), query, execute, collection->name());
}

void ExplorerTreeWidget::openCurrentDatabaseShell(const QString &script, bool execute)
{
    ExplorerDatabaseTreeItem *collectionItem = selectedDatabaseItem();
    if (!collectionItem)
        return;

    MongoDatabase *database = collectionItem->database();

    AppRegistry::instance().app()->
            openShell(database, script, execute, database->name());
}

void ExplorerTreeWidget::openCurrentServerShell(const QString &script, bool execute)
{
    ExplorerServerTreeItem *serverItem = selectedServerItem();
    if (!serverItem)
        return;

    MongoServer *server = serverItem->server();

    AppRegistry::instance().app()->
            openShell(server, script, QString(), execute, server->connectionRecord()->getReadableName());
}

void ExplorerTreeWidget::ui_disconnectServer()
{
    emit disconnectActionTriggered();
}

void ExplorerTreeWidget::ui_refreshServer()
{
    emit refreshActionTriggered();
}

void ExplorerTreeWidget::ui_openShell()
{
    emit openShellActionTriggered();
}

void ExplorerTreeWidget::ui_addDocument()
{
    openCurrentCollectionShell(
                "db.%1.save({\n"
                "    \"<key>\" : \"<value>\"\n"
                "});", false);
}

void ExplorerTreeWidget::ui_removeDocument()
{
    openCurrentCollectionShell(
                "db.%1.remove({ \"<field>\" : \"<value>\" });"
                , false);
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
    openCurrentDatabaseShell("db");
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
