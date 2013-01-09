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
        /*
        ** Constructs DatabaseTreeItem
        */
        ExplorerDatabaseTreeItem(MongoDatabase *database);
        ~ExplorerDatabaseTreeItem();

        MongoDatabase *database() const { return _database; }

        /*
        ** Expand database tree item to see collections;
        */
        void expandCollections();

    public slots:
        void vm_collectionRefreshed(const QList<MongoCollection *> &collections);
        void handle(MongoDatabase_CollectionListLoadedEvent *event);

    private:
        EventBus *_bus;

        ExplorerDatabaseCategoryTreeItem * _collectionItem;
        ExplorerDatabaseCategoryTreeItem * _javascriptItem;
        ExplorerDatabaseCategoryTreeItem * _usersItem;
        ExplorerDatabaseCategoryTreeItem * _filesItem;

        MongoDatabase *_database;
    };
}
