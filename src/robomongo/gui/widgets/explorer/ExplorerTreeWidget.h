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
        void openCurrentDatabaseShell(const QString &script, bool execute = true, const CursorPosition &cursor = CursorPosition());
        void openDatabaseShell(MongoDatabase *database, const QString &script, bool execute = true, const CursorPosition &cursor = CursorPosition());

    protected Q_SLOTS:
        void ui_refreshDatabase();
        void ui_createCollection();
        void ui_addUser();
        void ui_addFunction();
        void ui_editFunction();        
        void ui_dropFunction();
        void ui_refreshCollections();        
        
        void ui_addDocument();
        void ui_removeDocument();
        void ui_removeAllDocuments();

        void ui_addIndex();
        void ui_addIndexGui();
        void ui_reIndex();
        void ui_dropIndex();

        void ui_updateDocument();
        void ui_collectionStatistics();
        void ui_dropCollection();
        void ui_renameCollection();
        void ui_duplicateCollection();
        void ui_viewCollection();
        void ui_storageSize();
        void ui_totalIndexSize();
        void ui_totalSize();

        void ui_shardVersion();
        void ui_shardDistribution();

        void ui_dbStatistics();
        void ui_dbDrop();
        void ui_dbCollectionsStatistics();
        void ui_dbRepair();
        void ui_dbOpenShell();

        void ui_refreshUsers();
        void ui_refreshFunctions();
        void ui_viewUsers();
        void ui_viewFunctions();
        void ui_deleteIndex();
        void ui_viewIndex();
        void ui_refreshIndex();

    private:
        QMenu *_databaseContextMenu;
        QMenu *_collectionContextMenu;
        QMenu *_functionContextMenu;
        QMenu *_collectionCategoryContextMenu;
        QMenu *_usersCategoryContextMenu;
        QMenu *_functionsCategoryContextMenu;
        QMenu *_indexDirContextMenu;
        QMenu *_indexContextMenu;
    };
}
