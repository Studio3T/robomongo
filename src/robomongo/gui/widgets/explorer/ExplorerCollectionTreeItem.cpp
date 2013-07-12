#include "robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.h"

#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/gui/GuiRegistry.h"
namespace
{
	const QString tooltipTemplate = QString(
		"%0 "
		"<table>"
		"<tr><td>Count:</td> <td><b>&nbsp;&nbsp;%1</b></td></tr>"
		"<tr><td>Size:</td><td><b>&nbsp;&nbsp;%2</b></td></tr>"
		"</table>"
		);
}
namespace Robomongo
{
	class Indexes: public QTreeWidgetItem
	{
	public:
		explicit Indexes(const QString &val)
		{
			setText(0, val);
			setIcon(0, GuiRegistry::instance().indexIcon());
		}
	};

    ExplorerCollectionTreeItem::ExplorerCollectionTreeItem(MongoCollection *collection) :
        _collection(collection)
    {
        setText(0, _collection->name());
        setIcon(0, GuiRegistry::instance().collectionIcon());
        //setExpanded(true);
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        setToolTip(0, buildToolTip(collection));
    }
	void ExplorerCollectionTreeItem::expand()
	{
	}
    QString ExplorerCollectionTreeItem::buildToolTip(MongoCollection *collection)
    {	
		return tooltipTemplate.arg(collection->name()).arg(collection->info().count()).arg(collection->sizeString());
    }
}
