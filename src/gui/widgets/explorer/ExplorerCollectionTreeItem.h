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

        /*
        ** View model
        */
        //ExplorerCollectionViewModel * _viewModel;

        MongoCollectionPtr _collection;

    public:

        /*
        ** Constructs collection tree item
        */
        ExplorerCollectionTreeItem(const MongoCollectionPtr &collection);

    };
}


#endif // EXPLORERCOLLECTIONTREEITEM_H
