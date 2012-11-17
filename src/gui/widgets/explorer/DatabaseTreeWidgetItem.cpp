#include "StdAfx.h"
#include "DatabaseTreeWidgetItem.h"
#include "Mongo/MongoCollectionOld.h"
#include "Mongo/MongoDatabaseOld.h"
#include "DatabaseCategoryWidgetItem.h"

/************************************************************************/
/* DatabaseTreeWidgetItem                                               */
/************************************************************************/

DatabaseTreeWidgetItem::DatabaseTreeWidgetItem(MongoDatabaseOld * database) : QObject()
{
	_database = database;
	QIcon icon = qApp->style()->standardIcon(QStyle::SP_DirIcon);

	_collectionItem = new DatabaseCategoryWidgetItem(Collections, database);
	_collectionItem->setText(0, "Collections");
	_collectionItem->setIcon(0, icon);
	_collectionItem->setExpanded(true);
	_collectionItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
	addChild(_collectionItem);

	_javascriptItem = new DatabaseCategoryWidgetItem(Functions, database);
	_javascriptItem->setText(0, "Functions");
	_javascriptItem->setIcon(0, icon);
	_javascriptItem->setExpanded(true);
	_javascriptItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
	addChild(_javascriptItem);

	_filesItem = new DatabaseCategoryWidgetItem(Files, database);
	_filesItem->setText(0, "Files");
	_filesItem->setIcon(0, icon);
	_filesItem->setExpanded(true);
	_filesItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
	addChild(_filesItem);

	_usersItem = new DatabaseCategoryWidgetItem(Users, database);
	_usersItem->setText(0, "Users");
	_usersItem->setIcon(0, icon);
	_usersItem->setExpanded(true);
	_usersItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
	addChild(_usersItem);
}

DatabaseTreeWidgetItem::~DatabaseTreeWidgetItem()
{

}

void DatabaseTreeWidgetItem::refreshCollections(MongoDatabaseOld * database)
{
	int itemCount = _collectionItem->childCount();

	// remove child items
	for (int i = 0; i < itemCount; ++i)
	{
		QTreeWidgetItem * p = _collectionItem->child(0);
		_collectionItem->removeChild(p);
		delete p;
	}

	QIcon icon = qApp->style()->standardIcon(QStyle::SP_DirHomeIcon);
		
	foreach(MongoCollectionOld * mongoCollection, database->collections())
	{
		CollectionTreeWidgetItem * dbItem = new CollectionTreeWidgetItem(database, mongoCollection);
		dbItem->setText(0, mongoCollection->collectionName());
		dbItem->setIcon(0, icon);
		dbItem->setExpanded(true);

		// storing pointer to server model
		dbItem->setData(0, Qt::UserRole, QVariant::fromValue<void*>(mongoCollection));
		_collectionItem->addChild(dbItem);
	}
}

/************************************************************************/
/*                                                                      */
/************************************************************************/

CollectionTreeWidgetItem::CollectionTreeWidgetItem(MongoDatabaseOld * database, MongoCollectionOld * collection) : QObject()
{
	_database = database;
	_collection = collection;
	QIcon icon = qApp->style()->standardIcon(QStyle::SP_DirIcon);
}

CollectionTreeWidgetItem::~CollectionTreeWidgetItem()
{

}
