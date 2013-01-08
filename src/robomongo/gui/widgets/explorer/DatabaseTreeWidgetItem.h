#ifndef DATABASETREEWIDGETITEM_H
#define DATABASETREEWIDGETITEM_H

#include <QObject>

class MongoDatabaseOld;
class MongoCollectionOld;
class DatabaseCategoryWidgetItem;

class DatabaseTreeWidgetItem : public QObject, public QTreeWidgetItem
{
	Q_OBJECT

public:
	DatabaseTreeWidgetItem(MongoDatabaseOld * database);
	~DatabaseTreeWidgetItem();

	MongoDatabaseOld * database() const { return _database; }

public slots:
	void refreshCollections(MongoDatabaseOld * database);

private:

	MongoDatabaseOld * _database;

	DatabaseCategoryWidgetItem * _collectionItem;
	DatabaseCategoryWidgetItem * _javascriptItem;
	DatabaseCategoryWidgetItem * _usersItem;
	DatabaseCategoryWidgetItem * _filesItem;

};



class CollectionTreeWidgetItem : public QObject, public QTreeWidgetItem
{
	Q_OBJECT

public:
	CollectionTreeWidgetItem(MongoDatabaseOld * database, MongoCollectionOld * collection);
	~CollectionTreeWidgetItem();

	MongoDatabaseOld * database() const { return _database; }
	MongoCollectionOld * collection() const { return _collection; }

private:
	MongoDatabaseOld * _database;
	MongoCollectionOld * _collection;
};


#endif // DATABASETREEWIDGETITEM_H
