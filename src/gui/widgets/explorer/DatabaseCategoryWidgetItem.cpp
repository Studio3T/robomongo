#include "StdAfx.h"
#include "DatabaseCategoryWidgetItem.h"

DatabaseCategoryWidgetItem::DatabaseCategoryWidgetItem(DatabaseCategory category, MongoDatabaseOld * database) : QObject()
{
	_database = database;
	_category = category;
}

DatabaseCategoryWidgetItem::~DatabaseCategoryWidgetItem()
{

}
