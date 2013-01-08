#include "StdAfx.h"
#include "JsonListWidget.h"
#include <QListWidget>
#include "JsonWidget.h"

JsonListWidget::JsonListWidget(QList<BSONObj> bsonObjects, QWidget *parent)	: QListWidget(parent)
{
	_bsonObjects = bsonObjects;

	foreach(BSONObj bsonObject, bsonObjects)
	{
		QListWidgetItem * item = new QListWidgetItem();

		item->setSizeHint(QSize(0, 36));
		JsonWidget * jsonWidget = new JsonWidget(bsonObject, this);
		jsonWidget->setCustomData(item);

		// storing pointer to our connection record
		item->setData(Qt::UserRole, QVariant::fromValue<void*>(jsonWidget));

		connect(jsonWidget, SIGNAL(geometryChanged(JsonWidget *, QSize &)), this, SLOT(changeItemGeometry(JsonWidget *, QSize &)));

		addItem(item);
		setItemWidget(item, jsonWidget);
	}

	setVerticalScrollMode(ScrollPerPixel);
	connect(this, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(jsonItemClicked(QListWidgetItem*)));
}

JsonListWidget::~JsonListWidget()
{

}

void JsonListWidget::changeItemGeometry(JsonWidget * jsonWidget, QSize & size)
{
	void * p = jsonWidget->customData();

	if (!p)	return;

	QListWidgetItem * item = reinterpret_cast<QListWidgetItem *>(p);
	item->setSizeHint(QSize(size.width(), size.height()));
}

void JsonListWidget::jsonItemClicked(QListWidgetItem * currentItem)
{
	// Do nothing if no item selected
	if (currentItem == 0)
		return;

	// Getting stored pointer
	QVariant data = currentItem->data(Qt::UserRole);
	JsonWidget * jsonWidget = reinterpret_cast<JsonWidget *>(data.value<void*>());

	jsonWidget->toggle();
}
