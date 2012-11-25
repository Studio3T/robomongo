#include "ExplorerServerTreeItem.h"
#include "ExplorerDatabaseTreeItem.h"
#include "GuiRegistry.h"
#include "mongodb/MongoServer.h"
#include "mongodb/MongoDatabase.h"
#include "settings/ConnectionRecord.h"
#include "AppRegistry.h"
#include "Dispatcher.h"

using namespace Robomongo;

ExplorerServerTreeItem::ExplorerServerTreeItem(const MongoServerPtr &server) : QObject(),
    _server(server),
    _dispatcher(AppRegistry::instance().dispatcher())

{
    _dispatcher.subscribe(this, DatabaseListLoadedEvent::EventType, _server.get());

    setText(0, _server->connectionRecord()->getReadableName());
    setIcon(0, GuiRegistry::instance().serverIcon());
	setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
}

ExplorerServerTreeItem::~ExplorerServerTreeItem()
{
    int z = 56;
}

bool ExplorerServerTreeItem::event(QEvent *event)
{
    R_HANDLE(event) {
        R_EVENT(DatabaseListLoadedEvent);
    }

    return QObject::event(event);
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

    // Add system folder
    QIcon folderIcon = GuiRegistry::instance().folderIcon();
    QTreeWidgetItem * systemFolder = new QTreeWidgetItem();
    systemFolder->setIcon(0, folderIcon);
    systemFolder->setText(0, "System");
    addChild(systemFolder);

// no leaks:
//    ExplorerDatabaseTreeItem * dbItem = new ExplorerDatabaseTreeItem(dbs.at(0));
//    addChild(dbItem);

    for (int i = 0; i < dbs.size(); i++)
    {
        // why leaks??
        MongoDatabasePtr database(dbs.at(i));

        if (database->isSystem())
        {
            ExplorerDatabaseTreeItem * dbItem = new ExplorerDatabaseTreeItem(database);
            systemFolder->addChild(dbItem);
            continue;
        }

        ExplorerDatabaseTreeItem * dbItem = new ExplorerDatabaseTreeItem(database);
        addChild(dbItem);
    }

   // foreach(MongoDatabasePtr database, dbs)
    //{

    //}
}

void ExplorerServerTreeItem::handle(DatabaseListLoadedEvent *event)
{
    databaseRefreshed(event->list);
}
