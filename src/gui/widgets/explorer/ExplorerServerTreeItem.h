#ifndef EXPLORERSERVERTREEITEM_H
#define EXPLORERSERVERTREEITEM_H

#include <QTreeWidgetItem>
#include <QObject>

namespace Robomongo
{
    class ExplorerServerTreeItem : public QObject, public QTreeWidgetItem
    {
        Q_OBJECT

    private:


    public:

        /*
        ** Constructs ExplorerServerTreeItem
        */
        ExplorerServerTreeItem();

        /*
        ** Expand server tree item;
        */
        void expand();


    public slots:

        /*
        **
        */
        void databaseRefreshed();


    };
}


#endif // EXPLORERSERVERTREEITEM_H
