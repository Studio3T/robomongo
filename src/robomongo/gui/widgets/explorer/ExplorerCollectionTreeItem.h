#pragma once

#include <QObject>
#include <QTreeWidgetItem>

#include "robomongo/core/Core.h"

namespace Robomongo
{
    class ExplorerCollectionTreeItem : public QTreeWidgetItem
    {
    public:
        ExplorerCollectionTreeItem(MongoCollection *collection);
        MongoCollection *collection() const { return _collection; }

    private:
        QString buildToolTip(MongoCollection *collection);
        MongoCollection *_collection;
    };
}
