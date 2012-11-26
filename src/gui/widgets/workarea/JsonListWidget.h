#ifndef JSONLISTWIDGET_H
#define JSONLISTWIDGET_H

#include <QListWidget>
#include <mongodb\client\dbclient.h>
#include "JsonWidget.h"
using namespace mongo;

class JsonListWidget : public QListWidget
{
	Q_OBJECT

public:
	JsonListWidget(QList<BSONObj> bsonObjects, QWidget *parent);
	~JsonListWidget();

public slots:
	void changeItemGeometry(JsonWidget * jsonWidget, QSize & size);
	void jsonItemClicked(QListWidgetItem * modelIndex);

private:
	QList<BSONObj> _bsonObjects;
};

#endif // JSONLISTWIDGET_H
