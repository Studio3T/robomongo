#ifndef DATABASECATEGORYWIDGETITEM_H
#define DATABASECATEGORYWIDGETITEM_H

#include <QObject>

enum DatabaseCategory 
{
	Collections,
	Functions,
	Files,
	Users
};

class MongoDatabaseOld;

class DatabaseCategoryWidgetItem : public QObject, public QTreeWidgetItem
{
	Q_OBJECT

public:
	DatabaseCategoryWidgetItem(DatabaseCategory category, MongoDatabaseOld * database);
	~DatabaseCategoryWidgetItem();

	DatabaseCategory category() const { return _category; }
	MongoDatabaseOld * database() const { return _database; }

private:
	DatabaseCategory _category;
	MongoDatabaseOld * _database;
};

#endif // DATABASECATEGORYWIDGETITEM_H
