#include "ExplorerServerTreeItem.h"
#include "ExplorerDatabaseTreeItem.h"
#include "GuiRegistry.h"
#include "mongodb/MongoServer.h"
#include "settings/ConnectionRecord.h"

using namespace Robomongo;

ExplorerServerTreeItem::ExplorerServerTreeItem(const MongoServerPtr &server) : QObject(),
    _server(server)
{
    setText(0, _server->connectionRecord()->getReadableName());
    setIcon(0, GuiRegistry::instance().serverIcon());
	setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

    //connect(_viewModel, SIGNAL(databasesRefreshed(QList<ExplorerDatabaseViewModel *>)), SLOT(databaseRefreshed(QList<ExplorerDatabaseViewModel *>)));
}

void ExplorerServerTreeItem::expand()
{
    QList<MongoDatabasePtr> dbs = _server->listDatabases();
    databaseRefreshed(dbs);
    //_viewModel->expand();
}

void ExplorerServerTreeItem::databaseRefreshed(QList<MongoDatabasePtr> dbs)
{
    // Remove child items
	int itemCount = childCount();
	for (int i = 0; i < itemCount; ++i)
	{
		QTreeWidgetItem * p = child(0);
		removeChild(p);
		delete p;
	}

    // Add system folder
    QIcon folderIcon = GuiRegistry::instance().folderIcon();
    QTreeWidgetItem * systemFolder = new QTreeWidgetItem();
    systemFolder->setIcon(0, folderIcon);
    systemFolder->setText(0, "System");
    addChild(systemFolder);

    foreach(MongoDatabasePtr database, dbs)
	{
        /*
        if (database->system())
        {
            ExplorerDatabaseTreeItem * dbItem = new ExplorerDatabaseTreeItem(database);
            systemFolder->addChild(dbItem);
            continue;
        }*/

        ExplorerDatabaseTreeItem * dbItem = new ExplorerDatabaseTreeItem(database);
		addChild(dbItem);
    }
}
