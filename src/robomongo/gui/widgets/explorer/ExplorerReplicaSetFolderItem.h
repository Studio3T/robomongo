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
        using BaseClass = ExplorerTreeItem;

        ExplorerReplicaSetFolderItem(ExplorerTreeItem *item, MongoServer *const server);

        void updateText();
        void disableSomeContextMenuActions();
        void expand();
        void setRefreshFlag(bool state) { _refreshFlag = state; }
        bool refreshFlag() const { return _refreshFlag; }

    private Q_SLOTS:
        void on_refresh();
        void on_repSetStatus();

        void handle(ReplicaSetFolderLoading *event);

    private:
        MongoServer *const _server = nullptr;
        bool _refreshFlag = true;
    };
}
