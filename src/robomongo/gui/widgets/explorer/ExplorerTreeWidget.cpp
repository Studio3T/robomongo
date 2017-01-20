#include "robomongo/gui/widgets/explorer/ExplorerTreeWidget.h"

#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"
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
       
        // If the replica set item is not reachable, do not show context menu
        auto replicaSetItem = dynamic_cast<ExplorerReplicaSetTreeItem*>(item);
        if (replicaSetItem && !replicaSetItem->isUp())
            return;

        // If the database set item is disabled, do not show context menu
        auto dbItem = dynamic_cast<ExplorerDatabaseTreeItem*>(item);
        if (dbItem && dbItem->isDisabled()) 
            return;

        if (item) {
            auto explorerItem = dynamic_cast<ExplorerTreeItem *>(item);
            if (explorerItem) 
                explorerItem->showContextMenuAtPos(mapToGlobal(event->pos()));
        }
    }
}
