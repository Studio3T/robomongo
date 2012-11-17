#ifndef EXPLORERSERVERTREEITEM_H
#define EXPLORERSERVERTREEITEM_H

#include <QObject>

class ExplorerServerViewModel;
class ExplorerDatabaseViewModel;

class ExplorerServerTreeItem : public QObject, public QTreeWidgetItem
{
	Q_OBJECT

private:

	/*
	** View model for explorer server tree item
	*/
	ExplorerServerViewModel * _viewModel;

public:

	/*
	** Constructs ExplorerServerTreeItem
	*/
	ExplorerServerTreeItem(ExplorerServerViewModel * viewModel);

	/*
	** Expand server tree item;
	*/
	void expand();

    /*
    ** View model for explorer server tree item
    */
    ExplorerServerViewModel * viewModel() const { return _viewModel; }

public slots:

	/*
	**
	*/
	void databaseRefreshed(QList<ExplorerDatabaseViewModel *> databases);

	
};

#endif // EXPLORERSERVERTREEITEM_H
