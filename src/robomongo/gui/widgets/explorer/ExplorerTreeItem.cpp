#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"

#include <QMenu>

namespace Robomongo
{
   ExplorerTreeItem::ExplorerTreeItem(QTreeWidgetItem *parent)
       :BaseClass(parent),_contextMenu(new QMenu() )
   {

   }

   ExplorerTreeItem::ExplorerTreeItem(QTreeWidget *view)
       :BaseClass(view),_contextMenu(new QMenu(view) )
   {

   }

   void ExplorerTreeItem::showContextMenuAtPos(const QPoint &pos)
   {
      _contextMenu->exec(pos);
   }

   ExplorerTreeItem::~ExplorerTreeItem()
   {
       _contextMenu->deleteLater();
   }
}
