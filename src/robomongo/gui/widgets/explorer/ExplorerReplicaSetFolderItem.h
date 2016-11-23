#pragma once

#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"

namespace Robomongo
{
    class MongoServer;
    struct ReplicaSetFolderLoading;

    class ExplorerReplicaSetFolderItem : public ExplorerTreeItem
    {
        Q_OBJECT
    public:
        typedef ExplorerTreeItem BaseClass;
        ExplorerReplicaSetFolderItem(ExplorerTreeItem *item, MongoServer *const server);

        // todo
        void updateText();

    private Q_SLOTS:
        void on_refresh();
        void on_repSetStatus();

        void handle(ReplicaSetFolderLoading *event);

    private:
        MongoServer *const _server;
    };
}
