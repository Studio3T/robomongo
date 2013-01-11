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
        "<table>"
        "<tr><td>Collection:</td> <td><b>%0</b></td></tr>"
        "<tr><td>Count:</td> <td><b>%1</b></td></tr>"
        "<tr><td>Size:</td><td><b>%2</b></td></tr>"
        "</table>"
        )
        .arg(collection->name())
        .arg(collection->info().count)
        .arg(collection->sizeNice());

    setToolTip(0, tooltip);
}
