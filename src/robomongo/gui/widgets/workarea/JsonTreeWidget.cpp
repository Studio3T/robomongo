#include "StdAfx.h"
#include "JsonTreeWidget.h"
#include <QTreeWidget>
#include <mongodb\bson\bsonobj.h>

/************************************************************************/
/* JsonTreeWidget                                                       */
/************************************************************************/

JsonTreeWidget::JsonTreeWidget(QWidget *parent) : QTreeWidget(parent)
{

}

JsonTreeWidget::~JsonTreeWidget()
{

}


/************************************************************************/
/* JsonTreeWidgetItem                                                   */
/************************************************************************/

JsonTreeWidgetItem::JsonTreeWidgetItem(BSONObj bsonObject) : QObject()
{
	_bsonObject = bsonObject;
}

JsonTreeWidgetItem::~JsonTreeWidgetItem()
{

}
