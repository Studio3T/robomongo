#ifndef EXPLORERDATABASECATEGORYTREEITEM_H
#define EXPLORERDATABASECATEGORYTREEITEM_H

#include <QTreeWidgetItem>

namespace Robomongo
{
    class ExplorerDatabaseTreeItem;

    enum ExplorerDatabaseCategory
    {
        Collections,
        Functions,
        Files,
        Users
    };

    /*
    ** Database category Tree Item (looks like folder in the UI)
    */
    class ExplorerDatabaseCategoryTreeItem : public QObject, public QTreeWidgetItem
    {
        Q_OBJECT

    private:

        /*
        ** Database category of this tree item
        */
        ExplorerDatabaseCategory _category;

        /*
        ** Database tree item this category belongs to
        */
        ExplorerDatabaseTreeItem * _databaseItem;

    public:

        /*
        ** Constructs database category tree item
        */
        ExplorerDatabaseCategoryTreeItem(ExplorerDatabaseCategory category, ExplorerDatabaseTreeItem * databaseItem);


        /*
        ** Database category of this tree item
        */
        ExplorerDatabaseCategory category() const { return _category; }

        /*
        ** Database tree item this category belongs to
        */
        ExplorerDatabaseTreeItem * databaseItem() const { return _databaseItem; }
    };

}


#endif // EXPLORERDATABASECATEGORYTREEITEM_H
