#ifndef EXPLORERDATABASETREEITEM_H
#define EXPLORERDATABASETREEITEM_H

#include <QObject>
#include <QTreeWidgetItem>


namespace Robomongo
{
    class ExplorerCollectionTreeItem;
    class ExplorerDatabaseCategoryTreeItem;

    class ExplorerDatabaseTreeItem : public QObject, public QTreeWidgetItem
    {
        Q_OBJECT

    private:

        ExplorerDatabaseCategoryTreeItem * _collectionItem;
        ExplorerDatabaseCategoryTreeItem * _javascriptItem;
        ExplorerDatabaseCategoryTreeItem * _usersItem;
        ExplorerDatabaseCategoryTreeItem * _filesItem;

    public:

        /*
        ** Constructs DatabaseTreeItem
        */
        ExplorerDatabaseTreeItem();

        /*
        ** Expand database tree item to see collections;
        */
        void expandCollections();

    public slots:

        void vm_collectionRefreshed();


    };
}

#endif // EXPLORERDATABASETREEITEM_H
