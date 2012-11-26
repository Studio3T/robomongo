#ifndef JSONTREEWIDGET_H
#define JSONTREEWIDGET_H

#include <QObject>
#include <mongodb\client\dbclient.h>
#include <QTreeWidgetItem>

using namespace mongo;

class QTreeWidget;

/************************************************************************/
/* JsonTreeWidget                                                       */
/************************************************************************/

class JsonTreeWidget : public QTreeWidget
{
	Q_OBJECT

public:
	JsonTreeWidget(QWidget *parent);
	~JsonTreeWidget();

private:
	
};

/************************************************************************/
/* JsonTreeWidgetItem                                                   */
/************************************************************************/

class JsonTreeWidgetItem : public QObject, public QTreeWidgetItem
{
	Q_OBJECT

public:
	JsonTreeWidgetItem(BSONObj bsonObject);
	~JsonTreeWidgetItem();

	BSONObj bsonObject() const { return _bsonObject; }

private:
	BSONObj _bsonObject;
};

#endif // JSONTREEWIDGET_H
