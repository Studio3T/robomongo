#ifndef BSONTREEWIDGET_H
#define BSONTREEWIDGET_H

#include <QTreeWidget>
#include "Mongo/Mongo.h"

using namespace Robomongo;

class BsonTreeWidget : public QTreeWidget
{
	Q_OBJECT

private:

	/*
	** Current set of documents
	*/
	QList<MongoDocument_Pointer> _documents;

public:

	/*
	** Constructs Bson Tree widget
	*/
	BsonTreeWidget(QWidget * parent);
	~BsonTreeWidget();

	/*
	** Set documents
	*/
	void setDocuments(const QList<MongoDocument_Pointer> & documents);

	QIcon getIcon(MongoElement_Pointer element);

public slots:

	/*
	** Handle itemExpanded event
	*/
	void ui_itemExpanded(QTreeWidgetItem * item);
};


#endif // BSONTREEWIDGET_H
