#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"

namespace Robomongo
{
   ExplorerTreeItem::ExplorerTreeItem(QTreeWidgetItem *parent)
       :BaseClass(parent),_contextMenu()
   {

   }
   ExplorerTreeItem::ExplorerTreeItem():BaseClass(),_contextMenu()
   {

   }
   void ExplorerTreeItem::showContextMenuAtPos(const QPoint &pos)
   {
       _contextMenu.exec(pos);
   }
}
