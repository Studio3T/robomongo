#ifndef EXPLORERCOLLECTIONTREEITEM_H
#define EXPLORERCOLLECTIONTREEITEM_H

#include <QObject>
#include <QTreeWidgetItem>
#include "Core.h"

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


#endif // EXPLORERCOLLECTIONTREEITEM_H
