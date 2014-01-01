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
    class ExplorerDatabaseCategoryTreeItem : public ExplorerTreeItem
    {
        Q_OBJECT
    public:
        typedef ExplorerTreeItem BaseClass;
        ExplorerDatabaseCategoryTreeItem(ExplorerDatabaseTreeItem *databaseItem,ExplorerDatabaseCategory category);        
        void expand();
        void retranslateUI();

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
        ExplorerDatabaseTreeItem *databaseItem() const;
        const ExplorerDatabaseCategory _category;
        
        QAction *_createCollectionAction;
        QAction *_dbCollectionsStatsAction;
        QAction *_refreshCollectionsAction;
        
        QAction *_refreshUsersAction;
        QAction *_viewUsersAction;
        QAction *_addUserAction;

        QAction *_refreshFunctionsAction;
        QAction *_viewFunctionsAction;
        QAction *_addFunctionAction;
    };
}
