#ifndef EXPLORERCOLLECTIONTREEITEM_H
#define EXPLORERCOLLECTIONTREEITEM_H

#include <QObject>
#include <QTreeWidgetItem>

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

    public:

        /*
        ** Constructs collection tree item
        */
        ExplorerCollectionTreeItem();

    };
}


#endif // EXPLORERCOLLECTIONTREEITEM_H
