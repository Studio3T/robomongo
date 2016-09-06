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
        ExplorerReplicaSetTreeItem(QTreeWidgetItem *parent, MongoServer *const server, const mongo::HostAndPort& repMemberHostAndPort,
                                   const bool isPrimary, const bool isUp);

        // Getters
        ConnectionSettings const* connectionSettings() const { return _connSettings.get(); }

    public Q_SLOTS:
        //void databaseRefreshed(const QList<MongoDatabase *> &dbs);
        //void handle(DatabaseListLoadedEvent *event);
        //void handle(MongoServerLoadingDatabasesEvent *event);

    private Q_SLOTS:
        void ui_openShell();
        void ui_openDirectConnection();
        void ui_refreshServer();
        void ui_serverHostInfo();
        void ui_serverStatus();
        void ui_serverVersion();
        void ui_showLog();

    private:
        mongo::HostAndPort _repMemberHostAndPort;
        const bool _isPrimary;
        const bool _isUp;

        MongoServer *const _server;
        std::unique_ptr<ConnectionSettings> _connSettings;
        EventBus *_bus; // todo: remove?
    };
}
