#include "robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.h"

#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/gui/GuiRegistry.h"

using namespace Robomongo;

ExplorerCollectionTreeItem::ExplorerCollectionTreeItem(MongoCollection *collection) : QObject(),
    _collection(collection)
{
    setText(0, _collection->name());
    setIcon(0, GuiRegistry::instance().collectionIcon());
	setExpanded(true);
	setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    setToolTip(0, buildToolTip(collection));
}

QString ExplorerCollectionTreeItem::buildToolTip(MongoCollection *collection)
{
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

    return tooltip;
}
