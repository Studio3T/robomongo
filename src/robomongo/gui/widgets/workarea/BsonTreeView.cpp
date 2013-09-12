#include "robomongo/gui/widgets/workarea/BsonTreeView.h"

#include <QHeaderView>
#include <QAction>
#include <QMenu>
#include <QKeyEvent>

#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"

#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"

namespace Robomongo
{
    BsonTreeView::BsonTreeView(MongoShell *shell, const MongoQueryInfo &queryInfo, QWidget *parent) 
        : BaseClass(parent),_notifier(this,shell,queryInfo)
    {
#if defined(Q_OS_MAC)
        setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
        GuiRegistry::instance().setAlternatingColor(this);
        setSelectionMode(QAbstractItemView::ExtendedSelection);
        setSelectionBehavior(QAbstractItemView::SelectRows);
        setIndentation(15);
        setContextMenuPolicy(Qt::CustomContextMenu);
        VERIFY(connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&))));

        _expandRecursive = new QAction("Expand Recursively", this);
         VERIFY(connect(_expandRecursive, SIGNAL(triggered()), SLOT(onExpandRecursive())));
         
        setStyleSheet("QTreeView { border-left: 1px solid #c7c5c4; border-top: 1px solid #c7c5c4; }");
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        header()->setSectionResizeMode(QHeaderView::Interactive);
#endif
    }

    void BsonTreeView::showContextMenu(const QPoint &point)
    {
        QModelIndex selectedInd = selectedIndex();
        if (selectedInd.isValid()){
            BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedInd);;

            QMenu menu(this);
            bool isSimple = false;
            if(documentItem){
                isSimple = detail::isSimpleType(documentItem);
                if (documentItem->childrenCount()){
                    menu.addAction(_expandRecursive);
                    menu.addSeparator();
                }
            }

            _notifier.initMenu(&menu,documentItem);

            QPoint menuPoint = mapToGlobal(point);
            menuPoint.setY(menuPoint.y() + header()->height());
            menu.exec(menuPoint);
        }
    }

    void BsonTreeView::resizeEvent(QResizeEvent *event)
    {
        BaseClass::resizeEvent(event);
        header()->resizeSections(QHeaderView::Stretch);
    }

    void BsonTreeView::keyPressEvent(QKeyEvent *event)
    {
        if (event->key()==Qt::Key_Delete){
            QModelIndexList indexses = selectionModel()->selectedRows();
            std::vector<BsonTreeItem*> items;
            bool isForce = event->modifiers() & Qt::ShiftModifier;
            for (QModelIndexList::const_iterator it = indexses.begin(); it!= indexses.end(); ++it)
            {
                BsonTreeItem *item = QtUtils::item<BsonTreeItem*>(*it);
                items.push_back(item);
            }
            _notifier.deleteDocuments(items,isForce);
        }
        return BaseClass::keyPressEvent(event);
    }

    void BsonTreeView::expandNode(const QModelIndex &index)
    {
        if(index.isValid()){
            BaseClass::expand(index);
            BsonTreeItem *item = QtUtils::item<BsonTreeItem*>(index);
            for(unsigned i = 0;i<item->childrenCount();++i){
                BsonTreeItem *tritem = item->child(i);
                if(tritem&&tritem->childrenCount()){
                    expandNode(model()->index(i,0,index));
                }
            }
        }
    }

    void BsonTreeView::onExpandRecursive()
    {
        expandNode(selectedIndex());
    }

    void BsonTreeView::handle(InsertDocumentResponse *event)
    {
        AppRegistry::instance().bus()->unsubscibe(this);
        _notifier.query();
    }

    /**
     * @returns selected BsonTreeItem, or NULL otherwise
     */
    QModelIndex BsonTreeView::selectedIndex() const
    {
        QModelIndexList indexses = selectionModel()->selectedRows();
        int count = indexses.count();

        if (indexses.count() != 1)
            return QModelIndex();

        return indexses[0];
    }
}
