#ifndef EXPLORERDATABASETREEITEM_H
#define EXPLORERDATABASETREEITEM_H

#include <QObject>

class ExplorerDatabaseViewModel;
class ExplorerCollectionTreeItem;
class ExplorerDatabaseCategoryTreeItem;

class ExplorerDatabaseTreeItem : public QObject, public QTreeWidgetItem
{
	Q_OBJECT

private:
	/*
	** View Model
	*/
	ExplorerDatabaseViewModel * _viewModel;

	ExplorerDatabaseCategoryTreeItem * _collectionItem;
	ExplorerDatabaseCategoryTreeItem * _javascriptItem;
	ExplorerDatabaseCategoryTreeItem * _usersItem;
	ExplorerDatabaseCategoryTreeItem * _filesItem;

public:

	/*
	** Constructs DatabaseTreeItem
	*/
	ExplorerDatabaseTreeItem(ExplorerDatabaseViewModel * _viewModel);

	/*
	** Expand database tree item to see collections;
	*/
	void expandCollections();

public slots:

	void vm_collectionRefreshed();

	
};

#endif // EXPLORERDATABASETREEITEM_H
