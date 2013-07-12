#include "robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.h"

#include "robomongo/core/AppRegistry.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/EventBus.h"

namespace
{
	const QString tooltipTemplate = QString(
		"%0 "
		"<table>"
		"<tr><td>Count:</td> <td><b>&nbsp;&nbsp;%1</b></td></tr>"
		"<tr><td>Size:</td><td><b>&nbsp;&nbsp;%2</b></td></tr>"
		"</table>"
		);
    void clearChildren(QTreeWidgetItem *root)
    {
        int itemCount = root->childCount();
        for (int i = 0; i < itemCount; ++i) {
            QTreeWidgetItem *item = root->child(0);
            root->removeChild(item);
            delete item;
        }
    }
    class Indexes: public QTreeWidgetItem
    {
    public:
        explicit Indexes(const QString &val)
        {
            setText(0, val);
            setIcon(0, Robomongo::GuiRegistry::instance().indexIcon());
        }
    };
}
namespace Robomongo
{
    ExplorerCollectionTreeItem::ExplorerCollectionTreeItem(ExplorerDatabaseTreeItem *const parent,MongoCollection *collection) :QObject(parent),
        _collection(collection)
    {
        AppRegistry::instance().bus()->subscribe(QObject::parent(), LoadCollectionIndexesResponse::Type, this);
        setText(0, _collection->name());
        setIcon(0, GuiRegistry::instance().collectionIcon());
        //setExpanded(true);
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        setToolTip(0, buildToolTip(collection));
    }
    void ExplorerCollectionTreeItem::handle(LoadCollectionIndexesResponse *event)
    {
        clearChildren(this);
        QList<QString> indexes = event->indexes();
        for(QList<QString>::const_iterator it=indexes.begin();it!=indexes.end();++it)
        {
            addChild(new Indexes(*it));
        }
    }
	void ExplorerCollectionTreeItem::expand()
	{
        (static_cast<ExplorerDatabaseTreeItem *const>(QObject::parent()))->expandColection(this);
	}
    QString ExplorerCollectionTreeItem::buildToolTip(MongoCollection *collection)
    {	
		return tooltipTemplate.arg(collection->name()).arg(collection->info().count()).arg(collection->sizeString());
    }
}
