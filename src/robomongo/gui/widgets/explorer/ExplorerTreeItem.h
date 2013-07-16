#pragma once
#include <QTreeWidgetItem>
#include <QMenu>

namespace Robomongo
{
    class ExplorerTreeItem : public QTreeWidgetItem
    {
    public:
        typedef QTreeWidgetItem BaseClass;
        ExplorerTreeItem();
        explicit ExplorerTreeItem(QTreeWidgetItem *parent);
        virtual void showContextMenuAtPos(const QPoint &pos);
    protected:
        QMenu _contextMenu;
    };
}
