#ifndef BSONTREEITEM_H
#define BSONTREEITEM_H

#include <QObject>
#include <QTreeWidget>
#include "Mongo/Mongo.h"

using namespace Robomongo;


/*
** BSON tree item (represents array or object)
*/
class BsonTreeItem : public QObject, public QTreeWidgetItem
{
	Q_OBJECT

private:
	/*
	** MongoDocument this tree item represents
	*/
	MongoElement * _element;

	/*
	** Document
	*/ 
	MongoDocument * _document;

	/*
	** Position in array. -1 if not in array
	*/
	int _position;

	/*
	** Setup item that represents bson document
	*/
	void setupDocument(MongoDocument * document);

	/*
	** Clean child items
	*/
	void cleanChildItems();

	QString buildObjectFieldName();
	QString buildFieldName();
	QString buildArrayFieldName(int itemsCount);
	QString buildSynopsis(QString text);

public:

	/*
	** Constructs BsonTreeItem
	*/
	BsonTreeItem(MongoElement * element, int position);
	~BsonTreeItem();

	/*
	** Constructs BsonTreeItem
	*/
	BsonTreeItem(MongoDocument * document, int position);

	/*
	** MongoDocument this tree item represents
	*/
	MongoElement * element() const { return _element; }

	MongoDocument * document() const;

	void expand();
};

#endif // BSONTREEITEM_H
