#pragma once

#include <QTreeWidget>
#include "robomongo/core/domain/CursorPosition.h"

namespace Robomongo
{
    class ExplorerCollectionTreeItem;
    class ExplorerDatabaseTreeItem;
    class ExplorerServerTreeItem;
    class ExplorerDatabaseCategoryTreeItem;
    class ExplorerUserTreeItem;
    class ExplorerFunctionTreeItem;
    class ExplorerCollectionDirIndexesTreeItem;
    class MongoDatabase;

    class ExplorerTreeWidget : public QTreeWidget
    {
        Q_OBJECT

    public:
        explicit ExplorerTreeWidget(QWidget *parent = 0);

    protected:
        void contextMenuEvent(QContextMenuEvent *event);

        ExplorerServerTreeItem *selectedServerItem();
        ExplorerCollectionTreeItem *selectedCollectionItem();
        ExplorerCollectionDirIndexesTreeItem *selectedCollectionDirIndexItem();
        ExplorerUserTreeItem *selectedUserItem();
        ExplorerFunctionTreeItem *selectedFunctionItem();
        ExplorerDatabaseTreeItem *selectedDatabaseItem();
        ExplorerDatabaseCategoryTreeItem *selectedDatabaseCategoryItem();
        void openCurrentCollectionShell(const QString &script, bool execute = true, const CursorPosition &cursor = CursorPosition());
        void openDatabaseShell(MongoDatabase *database, const QString &script, bool execute = true, const CursorPosition &cursor = CursorPosition());

    protected Q_SLOTS:
        void ui_createCollection();
        void ui_addUser();
        void ui_addFunction();
        void ui_refreshCollections();        

        void ui_addIndex();
        void ui_addIndexGui();
        void ui_reIndex();
        void ui_dropIndex();

        void ui_dbCollectionsStatistics();

        void ui_refreshUsers();
        void ui_refreshFunctions();
        void ui_viewUsers();
        void ui_viewFunctions();
        void ui_deleteIndex();
        void ui_viewIndex();
        void ui_refreshIndex();

    private:
        QMenu *_collectionCategoryContextMenu;
        QMenu *_usersCategoryContextMenu;
        QMenu *_functionsCategoryContextMenu;
        QMenu *_indexDirContextMenu;
        QMenu *_indexContextMenu;
    };
}
