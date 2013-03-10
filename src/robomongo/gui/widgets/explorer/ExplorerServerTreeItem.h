#pragma once

#include <QTreeWidgetItem>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/events/MongoEvents.h"

namespace Robomongo
{
    class EventBus;

    class ExplorerServerTreeItem : public QObject, public QTreeWidgetItem
    {
        Q_OBJECT

    public:
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

    public slots:
        void databaseRefreshed(const QList<MongoDatabase *> &dbs);

    public slots:
        void handle(DatabaseListLoadedEvent *event);
        void handle(MongoServer_LoadingDatabasesEvent *event);

    private:

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
