#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"

#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/Core.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseCategoryTreeItem.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/widgets/explorer/ExplorerUserTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerFunctionTreeItem.h"

using namespace Robomongo;

ExplorerDatabaseTreeItem::ExplorerDatabaseTreeItem(MongoDatabase *database) :
    QObject(),
    _database(database),
    _bus(AppRegistry::instance().bus())
{
    _bus->subscribe(this, MongoDatabase_CollectionListLoadedEvent::Type, _database);
    _bus->subscribe(this, MongoDatabase_UsersLoadedEvent::Type, _database);
    _bus->subscribe(this, MongoDatabase_FunctionsLoadedEvent::Type, _database);
    _bus->subscribe(this, MongoDatabase_CollectionsLoadingEvent::Type, _database);
    _bus->subscribe(this, MongoDatabase_FunctionsLoadingEvent::Type, _database);
    _bus->subscribe(this, MongoDatabase_UsersLoadingEvent::Type, _database);

    setText(0, _database->name());
    setIcon(0, GuiRegistry::instance().databaseIcon());
	setExpanded(true);
	setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

    _collectionFolderItem = new ExplorerDatabaseCategoryTreeItem(Collections, this);
    _collectionFolderItem->setText(0, buildCollectionsFolderName());
    _collectionFolderItem->setIcon(0, GuiRegistry::instance().folderIcon());
    _collectionFolderItem->setExpanded(true);
    _collectionFolderItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    addChild(_collectionFolderItem);

    _javascriptFolderItem = new ExplorerDatabaseCategoryTreeItem(Functions, this);
    _javascriptFolderItem->setText(0, buildFunctionsFolderName());
    _javascriptFolderItem->setIcon(0, GuiRegistry::instance().folderIcon());
    _javascriptFolderItem->setExpanded(true);
    _javascriptFolderItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    addChild(_javascriptFolderItem);

    // Files (GridFS) not implemented yet
    //_filesFolderItem = new ExplorerDatabaseCategoryTreeItem(Files, this);
    //_filesFolderItem->setText(0, "Files");
    //_filesFolderItem->setIcon(0, GuiRegistry::instance().folderIcon());
    //_filesFolderItem->setExpanded(true);
    //_filesFolderItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    //addChild(_filesFolderItem);

    _usersFolderItem = new ExplorerDatabaseCategoryTreeItem(Users, this);
    _usersFolderItem->setText(0, buildUsersFolderName());
    _usersFolderItem->setIcon(0, GuiRegistry::instance().folderIcon());
    _usersFolderItem->setExpanded(true);
    _usersFolderItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    addChild(_usersFolderItem);
}

void ExplorerDatabaseTreeItem::expandCollections()
{
    _database->loadCollections();
}

void ExplorerDatabaseTreeItem::expandUsers()
{
    _database->loadUsers();
}

void ExplorerDatabaseTreeItem::expandFunctions()
{
    _database->loadFunctions();
}

void ExplorerDatabaseTreeItem::handle(MongoDatabase_CollectionListLoadedEvent *event)
{
    QList<MongoCollection *> collections = event->collections;
    int count = collections.count();
    _collectionFolderItem->setText(0, buildCollectionsFolderName(&count));

    clearChildItems(_collectionFolderItem);
    createCollectionSystemFolderItem();

    for (int i = 0; i < collections.size(); ++i) {
        MongoCollection *collection = collections.at(i);

        if (collection->isSystem()) {
            addSystemCollectionItem(collection);
        } else {
            addCollectionItem(collection);
        }
    }

    showCollectionSystemFolderIfNeeded();
}

void ExplorerDatabaseTreeItem::handle(MongoDatabase_UsersLoadedEvent *event)
{
    QList<MongoUser> users = event->users();
    int count = users.count();
    _usersFolderItem->setText(0, buildUsersFolderName(&count));

    clearChildItems(_usersFolderItem);

    for (int i = 0; i < users.count(); ++i) {
        MongoUser user = users.at(i);
        addUserItem(event->database(), user);
    }
}

void ExplorerDatabaseTreeItem::handle(MongoDatabase_FunctionsLoadedEvent *event)
{
    QList<MongoFunction> functions = event->functions();
    int count = functions.count();
    _javascriptFolderItem->setText(0, buildFunctionsFolderName(&count));

    clearChildItems(_javascriptFolderItem);

    for (int i = 0; i < functions.count(); ++i) {
        MongoFunction fun = functions.at(i);
        addFunctionItem(event->database(), fun);
    }
}

void ExplorerDatabaseTreeItem::handle(MongoDatabase_CollectionsLoadingEvent *event)
{
    int count = -1;
    _collectionFolderItem->setText(0, buildCollectionsFolderName(&count));
}

void ExplorerDatabaseTreeItem::handle(MongoDatabase_FunctionsLoadingEvent *event)
{
    int count = -1;
    _javascriptFolderItem->setText(0, buildFunctionsFolderName(&count));
}

void ExplorerDatabaseTreeItem::handle(MongoDatabase_UsersLoadingEvent *event)
{
    int count = -1;
    _usersFolderItem->setText(0, buildUsersFolderName(&count));
}

void ExplorerDatabaseTreeItem::clearChildItems(QTreeWidgetItem *root)
{
    int itemCount = root->childCount();
    for (int i = 0; i < itemCount; ++i) {
        QTreeWidgetItem *item = root->child(0);
        root->removeChild(item);
        delete item;
    }
}

void ExplorerDatabaseTreeItem::createCollectionSystemFolderItem()
{
    _collectionSystemFolderItem = new QTreeWidgetItem();
    _collectionSystemFolderItem->setIcon(0, GuiRegistry::instance().folderIcon());
    _collectionSystemFolderItem->setText(0, "System");
    _collectionFolderItem->addChild(_collectionSystemFolderItem);
}

void ExplorerDatabaseTreeItem::addCollectionItem(MongoCollection *collection)
{
    ExplorerCollectionTreeItem *collectionItem = new ExplorerCollectionTreeItem(collection);
    _collectionFolderItem->addChild(collectionItem);
}

void ExplorerDatabaseTreeItem::addSystemCollectionItem(MongoCollection *collection)
{
    ExplorerCollectionTreeItem *collectionItem = new ExplorerCollectionTreeItem(collection);
    _collectionSystemFolderItem->addChild(collectionItem);
}

void ExplorerDatabaseTreeItem::showCollectionSystemFolderIfNeeded()
{
    _collectionSystemFolderItem->setHidden(_collectionSystemFolderItem->childCount() == 0);
}

QString ExplorerDatabaseTreeItem::buildCollectionsFolderName(int *count /* = NULL */)
{
    if (!count)
        return "Collections";

    if (*count == -1)
        return "Collections ...";

    return QString("Collections (%1)").arg(*count);
}

QString ExplorerDatabaseTreeItem::buildUsersFolderName(int *count)
{
    if (!count)
        return "Users";

    if (*count == -1)
        return "Users ...";

    return QString("Users (%1)").arg(*count);
}

QString ExplorerDatabaseTreeItem::buildFunctionsFolderName(int *count)
{
    if (!count)
        return "Functions";

    if (*count == -1)
        return "Functions ...";

    return QString("Functions (%1)").arg(*count);
}

void ExplorerDatabaseTreeItem::addUserItem(MongoDatabase *database, const MongoUser &user)
{
    ExplorerUserTreeItem *userItem = new ExplorerUserTreeItem(database, user);
    _usersFolderItem->addChild(userItem);
}

void ExplorerDatabaseTreeItem::addFunctionItem(MongoDatabase *database, const MongoFunction &function)
{
    ExplorerFunctionTreeItem *functionItem = new ExplorerFunctionTreeItem(database, function);
    _javascriptFolderItem->addChild(functionItem);
}
