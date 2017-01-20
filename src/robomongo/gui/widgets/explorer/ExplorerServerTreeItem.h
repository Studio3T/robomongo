#pragma once

#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"

namespace Robomongo
{
    class EventBus;
    class MongoServerLoadingDatabasesEvent;
    class ExplorerReplicaSetFolderItem;
    class ExplorerTreeItem;

    class ExplorerServerTreeItem : public ExplorerTreeItem
    {
        Q_OBJECT

    public:
        typedef ExplorerTreeItem BaseClass;

        /*
        ** Constructs ExplorerServerTreeItem
        */
        ExplorerServerTreeItem(QTreeWidget *view, MongoServer *const server, ConnectionInfo connInfo);

        /*
        ** Expand server tree item;
        */
        void expand();

        // Disable/enable menu items, except [1]:Refresh and [9]:Disconnect which are 
        // always enabled, according to replica set status (online or offline).
        void disableSomeContextMenuActions(bool disable);

    public Q_SLOTS:
        void databaseRefreshed(const QList<MongoDatabase *> &dbs);
        void handle(DatabaseListLoadedEvent *event);
        void handle(MongoServerLoadingDatabasesEvent *event);
        void handle(ReplicaSetFolderRefreshed *event);

        // Special handle for server refresh events for replica set connections only
        void handle(ConnectionEstablishedEvent *event);

        // Special handle for server refresh events for replica set connections only
        void handle(ConnectionFailedEvent *event);

    private Q_SLOTS:
        void ui_showLog();
        void ui_openShell();
        void ui_disconnectServer();
        void ui_refreshServer();
        void ui_createDatabase();
        void ui_serverHostInfo();
        void ui_serverStatus();
        void ui_serverVersion();

    private:

        // Build all items for a root replica set server item, only used in refresh events
        // (not designed to be used in primary connection)
        void buildReplicaSetServerItem();

        // Build only replica set folder and member items
        void buildReplicaSetFolder(bool expanded);

        // This function assumes there is no existing db items (system folder and other db tree items), 
        // so existing db items should be deleted before calling this function.
        void buildDatabaseItems();  

        void replicaSetPrimaryReachable();
        void replicaSetPrimaryUnreachable();

        /**
         * @brief Builds server
         * @param count: Number of databases.
         *               If NULL - name will be without count of databases.
         *               If -1   - name will contain "..." at the end.
         */
        QString buildServerName(int *count = NULL, bool isOnline = true);

        ExplorerReplicaSetFolderItem *_replicaSetFolder;
        ExplorerTreeItem *_systemFolder;

        MongoServer *const _server;
        EventBus *_bus;

        // Flag to track last replica set connection's primary status
        bool _primaryWasUnreachable = false;
    };
}
