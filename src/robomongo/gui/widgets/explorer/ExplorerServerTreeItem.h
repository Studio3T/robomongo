#pragma once

#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"
#include "robomongo/core/events/MongoEventsInfo.hpp"

namespace Robomongo
{
    class MongoDatabase;
    class MongoServer;
    class MongoServerLoadingDatabasesEvent;
    
    class ExplorerServerTreeItem : public ExplorerTreeItem
    {
        Q_OBJECT

    public:
        typedef ExplorerTreeItem BaseClass;

        /*
        ** Constructs ExplorerServerTreeItem
        */
        ExplorerServerTreeItem(QTreeWidget *view, MongoServer *const server);

        /*
        ** Expand server tree item;
        */
        void expand();

    public Q_SLOTS:        
        void startLoadDatabases(const EventsInfo::LoadDatabaseNamesRequestInfo &inf);
        void addDatabases(const EventsInfo::LoadDatabaseNamesResponceInfo &inf);

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

        /**
         * @brief Builds server
         * @param count: Number of databases.
         *               If NULL - name will be without count of databases.
         *               If -1   - name will contain "..." at the end.
         */
        QString buildServerName(int *count = NULL);

        MongoServer *const _server;
    };
}
