#include "robomongo/gui/widgets/explorer/ExplorerDatabaseCategoryTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"

namespace Robomongo
{

ExplorerDatabaseCategoryTreeItem::ExplorerDatabaseCategoryTreeItem(ExplorerDatabaseCategory category,
                                                                   ExplorerDatabaseTreeItem *databaseItem) :
    _category(category),
    BaseClass(databaseItem) {}

ExplorerDatabaseTreeItem *ExplorerDatabaseCategoryTreeItem::databaseItem() const { return static_cast<ExplorerDatabaseTreeItem*>(BaseClass::parent()); }
}
