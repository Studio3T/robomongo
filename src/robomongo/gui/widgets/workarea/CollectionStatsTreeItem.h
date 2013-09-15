#pragma once
#include <QTreeWidgetItem>
#include "robomongo/core/Core.h"

namespace Robomongo
{
    class CollectionStatsTreeItem : public QTreeWidgetItem
    {
    public:
        CollectionStatsTreeItem(MongoDocumentPtr document);
    };
}
