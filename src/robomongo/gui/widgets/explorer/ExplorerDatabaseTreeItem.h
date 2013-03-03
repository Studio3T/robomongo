#pragma once

#include <QTreeWidgetItem>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoUser.h"

namespace Robomongo
{
    class ExplorerCollectionTreeItem;
    class ExplorerDatabaseCategoryTreeItem;
    class EventBus;
    class MongoDatabase_CollectionListLoadedEvent;
    class MongoDatabase_UsersLoadedEvent;

    class ExplorerDatabaseTreeItem : public QObject, public QTreeWidgetItem
    {
        Q_OBJECT

    public:
        ExplorerDatabaseTreeItem(MongoDatabase *database);
        ~ExplorerDatabaseTreeItem() {}

        MongoDatabase *database() const { return _database; }
        void expandCollections();
        void expandUsers();

    public slots:
        void handle(MongoDatabase_CollectionListLoadedEvent *event);
        void handle(MongoDatabase_UsersLoadedEvent *event);

    private:
        void clearChildItems(QTreeWidgetItem *root);
        void createCollectionSystemFolderItem();
        void addCollectionItem(MongoCollection *collection);
        void addSystemCollectionItem(MongoCollection *collection);
        void showCollectionSystemFolderIfNeeded();

        void addUserItem(MongoDatabase *database, const MongoUser &user);

        EventBus *_bus;
        ExplorerDatabaseCategoryTreeItem *_collectionFolderItem;
        ExplorerDatabaseCategoryTreeItem *_javascriptFolderItem;
        ExplorerDatabaseCategoryTreeItem *_usersFolderItem;
        ExplorerDatabaseCategoryTreeItem *_filesFolderItem;
        QTreeWidgetItem *_collectionSystemFolderItem;
        MongoDatabase *_database;
    };
}
