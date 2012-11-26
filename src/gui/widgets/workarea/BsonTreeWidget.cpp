#include "StdAfx.h"
#include "BsonTreeWidget.h"
#include "Mongo/Mongo.h"
#include "Mongo/MongoDocument.h"
#include "Mongo/MongoElement.h"
#include "Mongo/MongoDocumentIterator.h"
#include "AppRegistry.h"
#include "BsonTreeItem.h"

using namespace Robomongo;

BsonTreeWidget::BsonTreeWidget(QWidget * parent) : QTreeWidget(parent)
{
	QStringList colums;
	colums << "Key" << "Value" << "Type";
	setHeaderLabels(colums);
	header()->setResizeMode(0, QHeaderView::Stretch);
	header()->setResizeMode(1, QHeaderView::Stretch);
	header()->setResizeMode(2, QHeaderView::Stretch);
	setIndentation(15);	

//    header()->setResizeMode(QHeaderView::Interactive);

	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	connect(this, SIGNAL(itemExpanded(QTreeWidgetItem *)), SLOT(ui_itemExpanded(QTreeWidgetItem *)));
}

BsonTreeWidget::~BsonTreeWidget()
{
	
}

void BsonTreeWidget::setDocuments(const QList<MongoDocument_Pointer> & documents)
{
	_documents = documents;

	setUpdatesEnabled(false);
	clear();

    BsonTreeItem * firstItem = NULL;

	QList<QTreeWidgetItem *> items;
	for (int i = 0; i < documents.count(); i++)
	{
		MongoDocument_Pointer document = documents.at(i);

		BsonTreeItem * item = new BsonTreeItem(document.get(), i);
		items.append(item);

        if (i == 0)
            firstItem = item;
	}

	addTopLevelItems(items);
	setUpdatesEnabled(true);

    if (firstItem)
    {
        firstItem->expand();
        firstItem->setExpanded(true);
    }
}

void BsonTreeWidget::ui_itemExpanded(QTreeWidgetItem * treeItem)
{
	BsonTreeItem * item = static_cast<BsonTreeItem *>(treeItem);
	item->expand();

/*	MongoDocumentIterator iterator(item->document());

	while(iterator.hasMore())
	{
		MongoElement_Pointer element = iterator.next();
		
		if (element->isSimpleType() || element->bsonElement().isNull())
		{
			QTreeWidgetItem * childItem = new QTreeWidgetItem;
			childItem->setText(0, element->fieldName());
			childItem->setText(1, element->stringValue());
			childItem->setIcon(0, getIcon(element));
			item->addChild(childItem);
		} 
		else if (element->isDocument())
		{
			BsonTreeItem * newitem = new BsonTreeItem(element->asDocument(), element->isArray());

			if (item->isArray()) //is in array
			{
				newitem->setText(0, QString("[%1]").arg(element->fieldName()));
			}
			else
			{
				QString fieldName;
				
				if (element->isArray())
					fieldName = QString("%1 [%2]").arg(element->fieldName()).arg(element->bsonElement().Array().size());
				else
					fieldName = QString("%1 {..}").arg(element->fieldName());;

				newitem->setText(0, fieldName);
			}
			
			newitem->setIcon(0, getIcon(element));
			newitem->setExpanded(true);
			newitem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
			item->addChild(newitem);
		}
	}
	*/
}

QIcon BsonTreeWidget::getIcon(MongoElement_Pointer element)
{
	if (element->isArray())
		return AppRegistry::instance().bsonArrayIcon();
	
	if (element->isDocument())
		return AppRegistry::instance().bsonObjectIcon();

	if (element->isSimpleType())
	{
/*		if (element->fieldName() == "_id")
			return AppRegistry::instance().bsonIdIcon();*/

		if (element->isString())
			return AppRegistry::instance().bsonStringIcon();

		if (element->bsonElement().type() == Timestamp || element->bsonElement().type() == Date)
			return AppRegistry::instance().bsonDateTimeIcon();

		if (element->bsonElement().type() == NumberInt || element->bsonElement().type() == NumberLong)
			return AppRegistry::instance().bsonIntegerIcon();

		if (element->bsonElement().type() == NumberDouble)
			return AppRegistry::instance().bsonIntegerIcon();

		if (element->bsonElement().type() == Bool)
			return AppRegistry::instance().bsonBooleanIcon();

		if (element->bsonElement().type() == BinData)
			return AppRegistry::instance().bsonBinaryIcon();
	}

	if (element->bsonElement().type() == jstNULL)
		return AppRegistry::instance().bsonNullIcon();

	return AppRegistry::instance().circleIcon();
}
