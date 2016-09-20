#pragma once

#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"
#include "robomongo/core/domain/CursorPosition.h"

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
        ExplorerDatabaseCategoryTreeItem(ExplorerDatabaseTreeItem *databaseItem, ExplorerDatabaseCategory category);
        void expand();

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
        void functionTextEditorAddAccepted();

    private:
        ExplorerDatabaseTreeItem *databaseItem() const;
        const ExplorerDatabaseCategory _category;
        void openCurrentCollectionShell(const QString &script, bool execute = true, const CursorPosition &cursor = CursorPosition());
    };
}
