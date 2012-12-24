#include "ExplorerCollectionTreeItem.h"
#include "GuiRegistry.h"
#include "domain/MongoCollection.h"

using namespace Robomongo;

/*
** Constructs collection tree item
*/
ExplorerCollectionTreeItem::ExplorerCollectionTreeItem(MongoCollection *collection) : QObject(),
    _collection(collection)
{
    setText(0, _collection->name());
    setIcon(0, GuiRegistry::instance().collectionIcon());
	setExpanded(true);
	setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
}
