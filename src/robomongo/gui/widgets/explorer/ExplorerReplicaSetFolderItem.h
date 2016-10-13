#pragma once

#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"

namespace Robomongo
{
    class MongoServer;

    class ExplorerReplicaSetFolderItem : public ExplorerTreeItem
    {
        Q_OBJECT
    public:
        typedef ExplorerTreeItem BaseClass;
        ExplorerReplicaSetFolderItem(ExplorerTreeItem *item, MongoServer *const server);

    private Q_SLOTS:
        void on_refresh();
        void on_repSetStatus();

    private:
        MongoServer *const _server;
    };
}
