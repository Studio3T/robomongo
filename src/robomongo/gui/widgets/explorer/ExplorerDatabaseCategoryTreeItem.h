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
        Q_OBJECT
    public:
        typedef ExplorerTreeItem BaseClass;
        ExplorerDatabaseCategoryTreeItem(ExplorerDatabaseCategory category, ExplorerDatabaseTreeItem *databaseItem);
        ExplorerDatabaseCategory category() const { return _category; }
        ExplorerDatabaseTreeItem *databaseItem() const;

    private Q_SLOTS:
        void ui_createCollection();
        void ui_addUser();
        void ui_addFunction();
        void ui_refreshCollections();        

        void ui_dbCollectionsStatistics();

        void ui_refreshUsers();
        void ui_refreshFunctions();
        void ui_viewUsers();
        void ui_viewFunctions();

    private:
        const ExplorerDatabaseCategory _category;
    };
}
