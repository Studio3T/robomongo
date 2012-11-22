#include "ExplorerCollectionTreeItem.h"
#include "GuiRegistry.h"
#include "mongodb/MongoCollection.h"

using namespace Robomongo;

/*
** Constructs collection tree item
*/
ExplorerCollectionTreeItem::ExplorerCollectionTreeItem(const MongoCollectionPtr &collection) : QObject(),
    _collection(collection)
{
    // _viewModel = viewModel;

    setText(0, _collection->name());
    setIcon(0, GuiRegistry::instance().collectionIcon());
	setExpanded(true);
	setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
}
