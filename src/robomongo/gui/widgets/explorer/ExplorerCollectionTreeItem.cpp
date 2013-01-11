#include "robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.h"

#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/gui/GuiRegistry.h"

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

    QString tooltip = QString(
        "%0 "
        "<table>"
        "<tr><td>Count:</td> <td><b>&nbsp; %1</b></td></tr>"
        "<tr><td>Size:</td><td><b>&nbsp; %2</b></td></tr>"
        "</table>"
        )
        .arg(collection->name())
        .arg(collection->info().count)
        .arg(collection->sizeString());

    setToolTip(0, tooltip);
}
