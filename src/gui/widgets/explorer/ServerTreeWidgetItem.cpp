#include "StdAfx.h"
#include "ServerTreeWidgetItem.h"
#include "Mongo/MongoServerOld.h"
#include "DatabaseTreeWidgetItem.h"

ServerTreeWidgetItem::ServerTreeWidgetItem()
{

}

ServerTreeWidgetItem::~ServerTreeWidgetItem()
{

}

void ServerTreeWidgetItem::refreshDatabases(MongoServerOld * server)
{
	// remove child items
	for (int i = 0; i < this->childCount(); ++i)
		this->removeChild(this->child(i));

	QIcon icon = qApp->style()->standardIcon(QStyle::SP_DirHomeIcon);

	foreach(MongoDatabaseOld * mongoDatabase, server->databases())
	{
		DatabaseTreeWidgetItem * dbItem = new DatabaseTreeWidgetItem(mongoDatabase);
		dbItem->setText(0, mongoDatabase->databaseName());
		dbItem->setIcon(0, icon);
		dbItem->setExpanded(true);

		// storing pointer to server model
		dbItem->setData(0, Qt::UserRole, QVariant::fromValue<void*>(mongoDatabase));
		dbItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
		addChild(dbItem);


		

		connect(mongoDatabase, SIGNAL(collectionsChanged(MongoDatabaseOld *)), dbItem, SLOT(refreshCollections(MongoDatabaseOld * )));
	}
}
