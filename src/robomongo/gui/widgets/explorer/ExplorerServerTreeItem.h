#pragma once

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"

namespace Robomongo
{
    class EventBus;

    class ExplorerServerTreeItem : public QObject, public ExplorerTreeItem
    {
        Q_OBJECT

    public:
        typedef ExplorerTreeItem BaseClass;
        /*
        ** Constructs ExplorerServerTreeItem
        */
        ExplorerServerTreeItem(MongoServer *server);
        ~ExplorerServerTreeItem();

        /*
        ** Expand server tree item;
        */
        void expand();

        MongoServer *server() const { return _server; }
    Q_SIGNALS:
        void disconnectActionTriggered();
        void openShellActionTriggered();

    public slots:
        void databaseRefreshed(const QList<MongoDatabase *> &dbs);

    public slots:
        void handle(DatabaseListLoadedEvent *event);
        void handle(MongoServerLoadingDatabasesEvent *event);
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
        void openCurrentServerShell(const QString &script, bool execute = true, const CursorPosition &cursor = CursorPosition());

        /**
         * @brief Builds server
         * @param count: Number of databases.
         *               If NULL - name will be without count of databases.
         *               If -1   - name will contain "..." at the end.
         */
        QString buildServerName(int *count = NULL);

        MongoServer *_server;
        EventBus *_bus;
    };
}
