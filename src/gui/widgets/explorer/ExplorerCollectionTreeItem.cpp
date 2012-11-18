#include "ExplorerCollectionTreeItem.h"
#include "GuiRegistry.h"

using namespace Robomongo;

/*
** Constructs collection tree item
*/
ExplorerCollectionTreeItem::ExplorerCollectionTreeItem() : QObject()
{
    // _viewModel = viewModel;

    setText(0, "Some collection name" /*_viewModel->collectionName()*/);
    setIcon(0, GuiRegistry::instance().collectionIcon());
	setExpanded(true);
	setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
}
