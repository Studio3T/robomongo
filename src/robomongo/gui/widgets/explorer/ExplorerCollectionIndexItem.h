#pragma once

#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"

#include "robomongo/core/events/MongoEventsInfo.h"

namespace Robomongo
{
    class ExplorerCollectionIndexesDir;

    class ExplorerCollectionIndexItem : public ExplorerTreeItem
    {
        Q_OBJECT

    public:
        typedef ExplorerTreeItem BaseClass;
        explicit ExplorerCollectionIndexItem(
            ExplorerCollectionIndexesDir *parent, const IndexInfo &info);

    private Q_SLOTS:
        void ui_dropIndex();
        void ui_edit();

    private:
        IndexInfo _info;
    };
}