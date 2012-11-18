#include "ExplorerDatabaseCategoryTreeItem.h"
#include "ExplorerDatabaseTreeItem.h"

using namespace Robomongo;

/*
** Constructs database category tree item
*/
ExplorerDatabaseCategoryTreeItem::ExplorerDatabaseCategoryTreeItem(ExplorerDatabaseCategory category, ExplorerDatabaseTreeItem * databaseItem) : QObject()
{
	_category = category;
	_databaseItem = databaseItem;
}
