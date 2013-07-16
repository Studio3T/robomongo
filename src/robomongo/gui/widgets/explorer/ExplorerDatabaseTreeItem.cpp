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
#include "robomongo/core/domain/MongoServer.h"

namespace
{
    QString buildName(const QString& text,int *count)
    {
        if (!count)
            return text;

        if (*count == -1)
            return text.arg(" ...");

        return QString("%1 (%2)").arg(text).arg(*count);
    }
}

namespace Robomongo
{
    ExplorerDatabaseTreeItem::ExplorerDatabaseTreeItem(MongoDatabase *database) :
        QObject(),
        _database(database),
        _bus(AppRegistry::instance().bus())
    {
        _bus->subscribe(this, MongoDatabaseCollectionListLoadedEvent::Type, _database);
        _bus->subscribe(this, MongoDatabaseUsersLoadedEvent::Type, _database);
        _bus->subscribe(this, MongoDatabaseFunctionsLoadedEvent::Type, _database);
        _bus->subscribe(this, MongoDatabaseCollectionsLoadingEvent::Type, _database);
        _bus->subscribe(this, MongoDatabaseFunctionsLoadingEvent::Type, _database);
        _bus->subscribe(this, MongoDatabaseUsersLoadingEvent::Type, _database);
        
        setText(0, _database->name());
        setIcon(0, GuiRegistry::instance().databaseIcon());
        setExpanded(true);
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

        _collectionFolderItem = new ExplorerDatabaseCategoryTreeItem(Collections, this);
        _collectionFolderItem->setText(0, buildName("Collections",0));
        _collectionFolderItem->setIcon(0, GuiRegistry::instance().folderIcon());
        _collectionFolderItem->setExpanded(true);
        _collectionFolderItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        addChild(_collectionFolderItem);

        _javascriptFolderItem = new ExplorerDatabaseCategoryTreeItem(Functions, this);
        _javascriptFolderItem->setText(0, buildName("Functions",0));
        _javascriptFolderItem->setIcon(0, GuiRegistry::instance().folderIcon());
        _javascriptFolderItem->setExpanded(true);
        _javascriptFolderItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        addChild(_javascriptFolderItem);
        
        _usersFolderItem = new ExplorerDatabaseCategoryTreeItem(Users, this);
        _usersFolderItem->setText(0, buildName("Users",0));
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

    void ExplorerDatabaseTreeItem::expandColection(ExplorerCollectionTreeItem *const item)
    {
         _bus->send(_database->server()->client(), new LoadCollectionIndexesRequest(item, item->collection()->info()));
    }

    void ExplorerDatabaseTreeItem::deleteIndexFromCollection(ExplorerCollectionTreeItem *const item,const QString& indexText)
    {
        _bus->send(_database->server()->client(), new DeleteCollectionIndexRequest(item, item->collection()->info(),indexText));
    }

    void ExplorerDatabaseTreeItem::enshureIndex(ExplorerCollectionTreeItem *const item,const QString& text,bool unique,bool backGround,bool dropDuplicateIndex)
    {
        _bus->send(_database->server()->client(), new EnsureIndexRequest(item, item->collection()->info(),text,unique,backGround,dropDuplicateIndex));
    }

    void ExplorerDatabaseTreeItem::expandFunctions()
    {
        _database->loadFunctions();
    }

    void ExplorerDatabaseTreeItem::handle(MongoDatabaseCollectionListLoadedEvent *event)
    {
        QList<MongoCollection *> collections = event->collections;
        int count = collections.count();
        _collectionFolderItem->setText(0, buildName("Collections",&count));

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

    void ExplorerDatabaseTreeItem::handle(MongoDatabaseUsersLoadedEvent *event)
    {
        QList<MongoUser> users = event->users();
        int count = users.count();
        _usersFolderItem->setText(0, buildName("Users",&count));

        clearChildItems(_usersFolderItem);

        for (int i = 0; i < users.count(); ++i) {
            MongoUser user = users.at(i);
            addUserItem(event->database(), user);
        }
    }

    void ExplorerDatabaseTreeItem::handle(MongoDatabaseFunctionsLoadedEvent *event)
    {
        QList<MongoFunction> functions = event->functions();
        int count = functions.count();
        _javascriptFolderItem->setText(0,  buildName("Functions",&count));

        clearChildItems(_javascriptFolderItem);

        for (int i = 0; i < functions.count(); ++i) {
            MongoFunction fun = functions.at(i);
            addFunctionItem(event->database(), fun);
        }
    }

    void ExplorerDatabaseTreeItem::handle(MongoDatabaseCollectionsLoadingEvent *event)
    {
        int count = -1;
        _collectionFolderItem->setText(0, buildName("Collections",&count));
    }

    void ExplorerDatabaseTreeItem::handle(MongoDatabaseFunctionsLoadingEvent *event)
    {
        int count = -1;
        _javascriptFolderItem->setText(0, buildName("Functions",&count));
    }

    void ExplorerDatabaseTreeItem::handle(MongoDatabaseUsersLoadingEvent *event)
    {
        int count = -1;
        _usersFolderItem->setText(0, buildName("Users",&count));
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
        ExplorerCollectionTreeItem *collectionItem = new ExplorerCollectionTreeItem(this,collection);
        _collectionFolderItem->addChild(collectionItem);
    }

    void ExplorerDatabaseTreeItem::addSystemCollectionItem(MongoCollection *collection)
    {
        ExplorerCollectionTreeItem *collectionItem = new ExplorerCollectionTreeItem(this,collection);
        _collectionSystemFolderItem->addChild(collectionItem);
    }

    void ExplorerDatabaseTreeItem::showCollectionSystemFolderIfNeeded()
    {
        _collectionSystemFolderItem->setHidden(_collectionSystemFolderItem->childCount() == 0);
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
}
