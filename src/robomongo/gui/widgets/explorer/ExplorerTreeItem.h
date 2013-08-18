#pragma once

#include <QTreeWidgetItem>
QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

namespace Robomongo
{
    class ExplorerTreeItem :public QObject, public QTreeWidgetItem
    {
        Q_OBJECT
    public:
        typedef QTreeWidgetItem BaseClass;
        explicit ExplorerTreeItem(QTreeWidget *view);
        explicit ExplorerTreeItem(QTreeWidgetItem *parent);
        virtual void showContextMenuAtPos(const QPoint &pos);
        using BaseClass::parent;
        virtual ~ExplorerTreeItem();

    protected:
        QMenu *const _contextMenu;
    };
}
