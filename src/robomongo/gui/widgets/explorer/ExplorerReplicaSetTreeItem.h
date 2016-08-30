#pragma once

#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"

namespace Robomongo
{
    class EventBus;
    class MongoServerLoadingDatabasesEvent;

    class ExplorerReplicaSetTreeItem : public ExplorerTreeItem
    {
        Q_OBJECT

    public:
        typedef ExplorerTreeItem BaseClass;

        /*
        ** Constructs ExplorerReplicaSetTreeItem
        */
        ExplorerReplicaSetTreeItem(QTreeWidgetItem *parent, MongoServer *const server, const std::string& replicaMemberIpPort);

    public Q_SLOTS:
        //void databaseRefreshed(const QList<MongoDatabase *> &dbs);
        //void handle(DatabaseListLoadedEvent *event);
        //void handle(MongoServerLoadingDatabasesEvent *event);

    private Q_SLOTS:
        void ui_showLog();
        void ui_openShell();
        void ui_refreshServer();
        void ui_serverHostInfo();
        void ui_serverStatus();
        void ui_serverVersion();

    private:
        MongoServer *const _server;
        EventBus *_bus; // todo: remove?
    };
}
