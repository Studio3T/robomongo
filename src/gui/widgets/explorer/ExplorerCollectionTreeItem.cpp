#include "StdAfx.h"
#include "ExplorerCollectionTreeItem.h"
#include "ExplorerCollectionViewModel.h"
#include "AppRegistry.h"

/*
** Constructs collection tree item
*/
ExplorerCollectionTreeItem::ExplorerCollectionTreeItem(ExplorerCollectionViewModel * viewModel) : QObject()
{
	_viewModel = viewModel;

	setText(0, _viewModel->collectionName());
	setIcon(0, AppRegistry::instance().collectionIcon());
	setExpanded(true);
	setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
}
