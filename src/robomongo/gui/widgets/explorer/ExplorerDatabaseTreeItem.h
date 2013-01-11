#pragma once

#include <QTreeWidgetItem>

#include "robomongo/core/Core.h"

namespace Robomongo
{
    class ExplorerCollectionTreeItem;
    class ExplorerDatabaseCategoryTreeItem;
    class EventBus;
    class MongoDatabase_CollectionListLoadedEvent;

    class ExplorerDatabaseTreeItem : public QObject, public QTreeWidgetItem
    {
        Q_OBJECT

    public:
        ExplorerDatabaseTreeItem(MongoDatabase *database);
        ~ExplorerDatabaseTreeItem() {}

        MongoDatabase *database() const { return _database; }
        void expandCollections();

    public slots:
        void handle(MongoDatabase_CollectionListLoadedEvent *event);

    private:
        void clearCollectionFolderItems();
        void createCollectionSystemFolderItem();
        void addCollectionItem(MongoCollection *collection);
        void addSystemCollectionItem(MongoCollection *collection);
        void showCollectionSystemFolderIfNeeded();

        EventBus *_bus;
        ExplorerDatabaseCategoryTreeItem *_collectionFolderItem;
        ExplorerDatabaseCategoryTreeItem *_javascriptFolderItem;
        ExplorerDatabaseCategoryTreeItem *_usersFolderItem;
        ExplorerDatabaseCategoryTreeItem *_filesFolderItem;
        QTreeWidgetItem *_collectionSystemFolderItem;
        MongoDatabase *_database;
    };
}
