#include "ExplorerServerTreeItem.h"
#include "ExplorerDatabaseTreeItem.h"
#include "GuiRegistry.h"
#include "domain/MongoServer.h"
#include "domain/MongoDatabase.h"
#include "settings/ConnectionRecord.h"
#include "AppRegistry.h"
#include "Dispatcher.h"

using namespace Robomongo;

ExplorerServerTreeItem::ExplorerServerTreeItem(const MongoServerPtr &server) : QObject(),
    _server(server),
    _dispatcher(AppRegistry::instance().dispatcher())
{
    QObject *z = _server.get();
    _dispatcher.subscribe(this, DatabaseListLoadedEvent::Type, _server.get());

    setText(0, _server->connectionRecord()->getReadableName());
    setIcon(0, GuiRegistry::instance().serverIcon());
	setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
}

ExplorerServerTreeItem::~ExplorerServerTreeItem()
{
    int z = 56;
}

void ExplorerServerTreeItem::expand()
{
    _server->listDatabases();
    //databaseRefreshed(dbs);
    //_viewModel->expand();
}

void ExplorerServerTreeItem::databaseRefreshed(const QList<MongoDatabasePtr> &dbs)
{
    // Remove child items
	int itemCount = childCount();
	for (int i = 0; i < itemCount; ++i)
	{
		QTreeWidgetItem * p = child(0);
		removeChild(p);
		delete p;
	}

    // Add 'System' folder
    QIcon folderIcon = GuiRegistry::instance().folderIcon();
    QTreeWidgetItem * systemFolder = new QTreeWidgetItem();
    systemFolder->setIcon(0, folderIcon);
    systemFolder->setText(0, "System");
    addChild(systemFolder);

    for (int i = 0; i < dbs.size(); i++)
    {
        MongoDatabasePtr database(dbs.at(i));

        if (database->isSystem()) {
            ExplorerDatabaseTreeItem * dbItem = new ExplorerDatabaseTreeItem(database);
            systemFolder->addChild(dbItem);
            continue;
        }

        ExplorerDatabaseTreeItem * dbItem = new ExplorerDatabaseTreeItem(database);
        addChild(dbItem);
    }

    // Show 'System' folder only if it has items
    systemFolder->setHidden(systemFolder->childCount() == 0);
}

void ExplorerServerTreeItem::handle(DatabaseListLoadedEvent *event)
{
    databaseRefreshed(event->list);
}
