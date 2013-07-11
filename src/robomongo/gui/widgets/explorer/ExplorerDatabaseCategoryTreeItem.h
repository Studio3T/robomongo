#pragma once

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
    class ExplorerDatabaseCategoryTreeItem : public QTreeWidgetItem
    {
    public:
        ExplorerDatabaseCategoryTreeItem(ExplorerDatabaseCategory category, ExplorerDatabaseTreeItem *databaseItem);
        ExplorerDatabaseCategory category() const { return _category; }
        ExplorerDatabaseTreeItem *databaseItem() const { return _databaseItem; }

    private:
        ExplorerDatabaseCategory _category;
        ExplorerDatabaseTreeItem *_databaseItem;
    };
}
