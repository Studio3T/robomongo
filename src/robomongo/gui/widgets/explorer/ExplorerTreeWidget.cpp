#include "robomongo/gui/widgets/explorer/ExplorerTreeWidget.h"

#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerReplicaSetTreeItem.h"
#include <QContextMenuEvent>
#include <robomongo/gui/GuiRegistry.h>

namespace Robomongo
{
    ExplorerTreeWidget::ExplorerTreeWidget(QWidget *parent) : QTreeWidget(parent)
    {
    #if defined(Q_OS_MAC)
        setAttribute(Qt::WA_MacShowFocusRect, false);
        QPalette palet = palette();
        palet.setColor(QPalette::Active, QPalette::Highlight, QColor(16, 108, 214));
        setPalette(palet);
    #endif
        setContextMenuPolicy(Qt::DefaultContextMenu);
        setObjectName("explorerTree");
        setIndentation(15);
        setHeaderHidden(true);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setExpandsOnDoubleClick(false);
    }

    void ExplorerTreeWidget::contextMenuEvent(QContextMenuEvent *event)
    {
        QTreeWidgetItem *item = itemAt(event->pos());
        auto replicaSetItem = dynamic_cast<ExplorerReplicaSetTreeItem*>(item);
        // If the replica set item is down, hide context menu
        if (replicaSetItem && !replicaSetItem->isUp()) {
            return;
        }

        if (item) {
            ExplorerTreeItem *expItem = dynamic_cast<ExplorerTreeItem *>(item);
            if (expItem) {
                expItem->showContextMenuAtPos(mapToGlobal(event->pos()));
            }
        }
    }
}
