#pragma once

#include <QTreeWidgetItem>
QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

namespace Robomongo
{
    class ExplorerTreeItem : public QTreeWidgetItem
    {
    public:
        typedef QTreeWidgetItem BaseClass;
        explicit ExplorerTreeItem(QTreeWidget *view);
        explicit ExplorerTreeItem(QTreeWidgetItem *parent);
        virtual void showContextMenuAtPos(const QPoint &pos);
        virtual ~ExplorerTreeItem();
    protected:
        QMenu *const _contextMenu;
    };
}
