#ifndef EXPLORERDATABASETREEITEM_H
#define EXPLORERDATABASETREEITEM_H

#include <QObject>
#include <QTreeWidgetItem>
#include "Core.h"

namespace Robomongo
{
    class ExplorerCollectionTreeItem;
    class ExplorerDatabaseCategoryTreeItem;
    class EventBus;
    class MongoDatabase_CollectionListLoadedEvent;

    class ExplorerDatabaseTreeItem : public QObject, public QTreeWidgetItem
    {
        Q_OBJECT

    private:

        ExplorerDatabaseCategoryTreeItem * _collectionItem;
        ExplorerDatabaseCategoryTreeItem * _javascriptItem;
        ExplorerDatabaseCategoryTreeItem * _usersItem;
        ExplorerDatabaseCategoryTreeItem * _filesItem;

        MongoDatabase *_database;

    public:

        /*
        ** Constructs DatabaseTreeItem
        */
        ExplorerDatabaseTreeItem(MongoDatabase *database);
        ~ExplorerDatabaseTreeItem();

        /*
        ** Expand database tree item to see collections;
        */
        void expandCollections();

    public slots:

        void vm_collectionRefreshed(const QList<MongoCollection *> &collections);

    public slots:
        void handle(MongoDatabase_CollectionListLoadedEvent *event);

    private:
        EventBus *_bus;
    };
}

#endif // EXPLORERDATABASETREEITEM_H
