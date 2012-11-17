#include "StdAfx.h"
#include "ExplorerDatabaseCategoryTreeItem.h"
#include "ExplorerDatabaseTreeItem.h"

/*
** Constructs database category tree item
*/
ExplorerDatabaseCategoryTreeItem::ExplorerDatabaseCategoryTreeItem(ExplorerDatabaseCategory category, ExplorerDatabaseTreeItem * databaseItem) : QObject()
{
	_category = category;
	_databaseItem = databaseItem;
}