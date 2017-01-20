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
        using BaseClass = ExplorerTreeItem;

        /*
        ** Constructs ExplorerReplicaSetTreeItem
        */
        ExplorerReplicaSetTreeItem(QTreeWidgetItem *parent, MongoServer *const server, const mongo::HostAndPort& repMemberHostAndPort,
                                   const bool isPrimary, const bool isUp);

        /**
        * @brief Updates this widget's text and icon
        * @param isUp: true if this set member is reachable, false otherwise
        * @param isPrimary: true if this set member is primary, false otherwise
        */
        void updateTextAndIcon(bool isUp, bool isPrimary);

        // Getters
        ConnectionSettings* connectionSettings() { return _connSettings.get(); }
        bool isUp() const { return _isUp; }
        MongoServer* server() const { return _server; }

    private Q_SLOTS:
        void ui_openShell();
        void ui_openDirectConnection();
        void ui_serverHostInfo();
        void ui_serverStatus();
        void ui_serverVersion();
        void ui_showLog();

    private:
        mongo::HostAndPort _repMemberHostAndPort;
        
        bool _isPrimary;    // true if this set member is primary, false otherwise
        bool _isUp;         // true if this set member is reachable, false otherwise

        MongoServer *const _server;
        std::unique_ptr<ConnectionSettings> _connSettings;
        EventBus *_bus; // todo: remove?
    };
}
