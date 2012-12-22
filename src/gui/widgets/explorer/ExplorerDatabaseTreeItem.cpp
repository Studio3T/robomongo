#include "ExplorerDatabaseTreeItem.h"
#include "GuiRegistry.h"
#include "ExplorerCollectionTreeItem.h"
#include "ExplorerDatabaseCategoryTreeItem.h"
#include "domain/MongoDatabase.h"
#include "domain/MongoCollection.h"
#include "AppRegistry.h"
#include "Dispatcher.h"
#include "Core.h"

using namespace Robomongo;

/*
** Constructs DatabaseTreeItem
*/
ExplorerDatabaseTreeItem::ExplorerDatabaseTreeItem(const MongoDatabasePtr &database) : QObject(),
    _database(database),
    _dispatcher(AppRegistry::instance().dispatcher())
{
    _dispatcher.subscribe(this, MongoDatabase_CollectionListLoadedEvent::Type, _database.get());

    setText(0, _database->name());
    setIcon(0, GuiRegistry::instance().databaseIcon());
	setExpanded(true);

	setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

    QIcon icon = GuiRegistry::instance().folderIcon();

	_collectionItem = new ExplorerDatabaseCategoryTreeItem(Collections, this);
	_collectionItem->setText(0, "Collections");
	_collectionItem->setIcon(0, icon);
	_collectionItem->setExpanded(true);
	_collectionItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
	addChild(_collectionItem);

	_javascriptItem = new ExplorerDatabaseCategoryTreeItem(Functions, this);
	_javascriptItem->setText(0, "Functions");
	_javascriptItem->setIcon(0, icon);
	_javascriptItem->setExpanded(true);
	_javascriptItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
	addChild(_javascriptItem);

	_filesItem = new ExplorerDatabaseCategoryTreeItem(Files, this);
	_filesItem->setText(0, "Files");
	_filesItem->setIcon(0, icon);
	_filesItem->setExpanded(true);
	_filesItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
	addChild(_filesItem);

	_usersItem = new ExplorerDatabaseCategoryTreeItem(Users, this);
	_usersItem->setText(0, "Users");
	_usersItem->setIcon(0, icon);
	_usersItem->setExpanded(true);
	_usersItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    addChild(_usersItem);
}

ExplorerDatabaseTreeItem::~ExplorerDatabaseTreeItem()
{
    int a = 87;
}

/*
** Expand database tree item;
*/
void ExplorerDatabaseTreeItem::expandCollections()
{
    _database->listCollections();
    //_viewModel->expandCollections();
}

void ExplorerDatabaseTreeItem::vm_collectionRefreshed(const QList<MongoCollectionPtr> &collections)
{
	// remove child items
	int itemCount = _collectionItem->childCount();
	for (int i = 0; i < itemCount; ++i)
	{
		QTreeWidgetItem * p = _collectionItem->child(0);
		_collectionItem->removeChild(p);
		delete p;
	}

    // Add system folder
    QIcon folderIcon = GuiRegistry::instance().folderIcon();
    QTreeWidgetItem * systemFolder = new QTreeWidgetItem();
    systemFolder->setIcon(0, folderIcon);
    systemFolder->setText(0, "System");
    _collectionItem->addChild(systemFolder);


    for (int i = 0; i < collections.size(); i++)
    {
        MongoCollectionPtr collection(collections.at(i));

        if (collection->isSystem())
        {
            ExplorerCollectionTreeItem * collectionItem = new ExplorerCollectionTreeItem(collection);
            systemFolder->addChild(collectionItem);
            continue;
        }

		ExplorerCollectionTreeItem * collectionItem = new ExplorerCollectionTreeItem(collection);
		_collectionItem->addChild(collectionItem);
    }

    // Show 'System' folder only if it has items
    systemFolder->setHidden(systemFolder->childCount() == 0);
}

void ExplorerDatabaseTreeItem::handle(MongoDatabase_CollectionListLoadedEvent *event)
{
    vm_collectionRefreshed(event->list);
}
