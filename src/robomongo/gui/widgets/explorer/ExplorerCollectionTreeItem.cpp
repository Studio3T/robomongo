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
}
namespace Robomongo
{
    Indexes::Indexes(const QString &val,ExplorerCollectionTreeItem *const parent):QTreeWidgetItem(parent)
    {
        setText(0, val);
        setIcon(0, Robomongo::GuiRegistry::instance().indexIcon());
    }
    
    ExplorerCollectionTreeItem::ExplorerCollectionTreeItem(ExplorerDatabaseTreeItem *const parent,MongoCollection *collection) :
        QObject(parent),_collection(collection)
    {
        AppRegistry::instance().bus()->subscribe(QObject::parent(), LoadCollectionIndexesResponse::Type, this);
        AppRegistry::instance().bus()->subscribe(QObject::parent(), DeleteCollectionIndexResponse::Type, this);
        setText(0, _collection->name());
        setIcon(0, GuiRegistry::instance().collectionIcon());
        //setExpanded(true);
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        setToolTip(0, buildToolTip(collection));
    }

    void ExplorerCollectionTreeItem::handle(LoadCollectionIndexesResponse *event)
    {
        clearChildren(this);
        const QList<QString> &indexes = event->indexes();
        for(QList<QString>::const_iterator it=indexes.begin();it!=indexes.end();++it)
        {
            addChild(new Indexes(*it,this));
        }
    }

    void ExplorerCollectionTreeItem::handle(DeleteCollectionIndexResponse *event)
    {
        if(!event->index().isEmpty()){
            int itemCount = childCount();
            for (int i = 0; i < itemCount; ++i) {
                QTreeWidgetItem *item = child(i);
                if(item->text(0)==event->index())
                {
                    removeChild(item);
                    delete item;
                    break;
                }
            }
        }
    }

    void ExplorerCollectionTreeItem::expand()
    {
        (static_cast<ExplorerDatabaseTreeItem *const>(QObject::parent()))->expandColection(this);
    }

    void ExplorerCollectionTreeItem::deleteIndex(const QTreeWidgetItem * const ind)
    {
        (static_cast<ExplorerDatabaseTreeItem *const>(QObject::parent()))->deleteIndexFromCollection(this,ind->text(0));
    }

    QString ExplorerCollectionTreeItem::buildToolTip(MongoCollection *collection)
    {	
        return tooltipTemplate.arg(collection->name()).arg(collection->info().count()).arg(collection->sizeString());
    }
}
