#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"

#include <QMenu>

namespace Robomongo
{
    void clearChildItems(QTreeWidgetItem *root)
    {
        int itemCount = root->childCount();
        for (int i = 0; i < itemCount; ++i) {
            QTreeWidgetItem *item = root->child(0);
            root->removeChild(item);
            delete item;
        }
    }
    ExplorerTreeItem::ExplorerTreeItem(QTreeWidgetItem *parent)
        :QObject(),BaseClass(parent),_contextMenu(new QMenu(treeWidget()) )
    {

    }

    ExplorerTreeItem::ExplorerTreeItem(QTreeWidget *view)
        :QObject(view),BaseClass(view),_contextMenu(new QMenu(view) )
    {

    }

    void ExplorerTreeItem::showContextMenuAtPos(const QPoint &pos)
    {
        _contextMenu->exec(pos);
    }

    ExplorerTreeItem::~ExplorerTreeItem()
    {
        _contextMenu->deleteLater();
        clearChildItems(this);
    }
}
