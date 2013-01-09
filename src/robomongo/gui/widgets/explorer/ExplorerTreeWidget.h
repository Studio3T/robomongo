#pragma once

#include <QTreeWidget>

namespace Robomongo
{
    class ExplorerCollectionTreeItem;
    class ExplorerDatabaseTreeItem;
    class ExplorerServerTreeItem;

    class ExplorerTreeWidget : public QTreeWidget
    {
        Q_OBJECT

    public:
        explicit ExplorerTreeWidget(QWidget *parent = 0);
        ~ExplorerTreeWidget();

    protected:
        /*
        ** Virtual method that used to handle rights clicks
        */
        void contextMenuEvent(QContextMenuEvent *event);

        ExplorerServerTreeItem *selectedServerItem();
        ExplorerCollectionTreeItem *selectedCollectionItem();
        ExplorerDatabaseTreeItem *selectedDatabaseItem();
        void openCurrentCollectionShell(const QString &script, bool execute = true);
        void openCurrentDatabaseShell(const QString &script, bool execute = true);
        void openCurrentServerShell(const QString &script, bool execute = true);

    signals:

        void disconnectActionTriggered();
        void refreshActionTriggered();
        void openShellActionTriggered();

    protected slots:

        void ui_disconnectServer();
        void ui_refreshServer();
        void ui_openShell();
        void ui_addDocument();
        void ui_removeDocument();

        void ui_addIndex();
        void ui_reIndex();
        void ui_dropIndex();

        void ui_updateDocument();
        void ui_collectionStatistics();
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

        void ui_serverHostInfo();
        void ui_serverStatus();
        void ui_serverVersion();

    private:
        /*
        ** Server context menu
        */
        QMenu *_serverMenu;
        QMenu *_databaseMenu;
        QMenu *_collectionMenu;
    };
}
