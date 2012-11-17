#ifndef EXPLORERCOLLECTIONTREEITEM_H
#define EXPLORERCOLLECTIONTREEITEM_H

#include <QObject>
class ExplorerCollectionViewModel;

class ExplorerCollectionTreeItem : public QObject, public QTreeWidgetItem
{
	Q_OBJECT

private:

	/*
	** View model
	*/
	ExplorerCollectionViewModel * _viewModel;

public:

	/*
	** Constructs collection tree item
	*/
	ExplorerCollectionTreeItem(ExplorerCollectionViewModel * viewModel);

	/*
	** View model
	*/
	ExplorerCollectionViewModel * viewModel() const { return _viewModel; }
};

#endif // EXPLORERCOLLECTIONTREEITEM_H
