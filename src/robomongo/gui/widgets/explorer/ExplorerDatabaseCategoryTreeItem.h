#pragma once

#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"

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
    class ExplorerDatabaseCategoryTreeItem : public QObject, public ExplorerTreeItem
    {
    public:
        typedef ExplorerTreeItem BaseClass;
        ExplorerDatabaseCategoryTreeItem(ExplorerDatabaseCategory category, ExplorerDatabaseTreeItem *databaseItem);
        ExplorerDatabaseCategory category() const { return _category; }
        ExplorerDatabaseTreeItem *databaseItem() const;

    private:
        ExplorerDatabaseCategory _category;
    };
}
