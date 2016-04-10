#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"

#include <QMenu>

#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    ExplorerTreeItem::ExplorerTreeItem(QTreeWidgetItem *parent)
        :QObject(), BaseClass(parent), _contextMenu(new QMenu(treeWidget()) )
    {

    }

    ExplorerTreeItem::ExplorerTreeItem(QTreeWidget *view)
        :QObject(view), BaseClass(view), _contextMenu(new QMenu(view) )
    {

    }

    void ExplorerTreeItem::showContextMenuAtPos(const QPoint &pos)
    {
        _contextMenu->exec(pos);
    }

    ExplorerTreeItem::~ExplorerTreeItem()
    {
        _contextMenu->deleteLater();
        QtUtils::clearChildItems(this);
    }
}
