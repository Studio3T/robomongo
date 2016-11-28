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
        void disableSomeContextMenuActions(/*bool disable*/); // todo: rename toggle*
        void expand();
        void setRefreshFlag(bool state) { _refreshFlag = state; }
        bool refreshFlag() const { return _refreshFlag; }

    private Q_SLOTS:
        void on_refresh();
        void on_repSetStatus();

        void handle(ReplicaSetFolderLoading *event);

    private:
        MongoServer *const _server;
        bool _refreshFlag = true;
    };
}
