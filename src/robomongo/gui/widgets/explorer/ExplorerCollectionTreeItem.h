#pragma once

#include <QObject>
#include <QTreeWidgetItem>

#include "robomongo/core/Core.h"

namespace Robomongo
{
    class ExplorerCollectionTreeItem : public QObject, public QTreeWidgetItem
    {
        Q_OBJECT

    private:

        MongoCollection *_collection;

    public:

        /*
        ** Constructs collection tree item
        */
        ExplorerCollectionTreeItem(MongoCollection *collection);

        MongoCollection *collection() const { return _collection; }
    };
}
