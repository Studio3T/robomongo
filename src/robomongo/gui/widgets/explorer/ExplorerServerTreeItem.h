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

    public Q_SLOTS:
        void databaseRefreshed(const QList<MongoDatabase *> &dbs);
        void handle(DatabaseListLoadedEvent *event);
        void handle(MongoServerLoadingDatabasesEvent *event);
        // todo
        void handle(ReplicaSetFolderRefreshed *event);
        void handle(ReplicaSetRefreshed *event);
        void handle(ConnectionEstablishedEvent *event);
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

        // todo: 
        void buildServerItem();  // build root item for single server or replica set
        void buildReplicaSetFolder();
        // This function assumes there is no existing db items (system folder and other db tree items), 
        // so existing db items should be deleted before calling this function.
        void buildDatabaseItems();  
        void replicaSetPrimaryUnreachable();

        ExplorerReplicaSetFolderItem *_replicaSetFolder;
        ExplorerTreeItem *_systemFolder;

        /**
         * @brief Builds server
         * @param count: Number of databases.
         *               If NULL - name will be without count of databases.
         *               If -1   - name will contain "..." at the end.
         */
        QString buildServerName(int *count = NULL, bool isOnline = true);

        MongoServer *const _server;
        EventBus *_bus;

        // todo: @brief
        bool _primaryWasUnreachable;
    };
}
