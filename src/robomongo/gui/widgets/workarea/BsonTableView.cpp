#include "robomongo/gui/widgets/workarea/BsonTableView.h"

#include <QHeaderView>
#include <QAction>
#include <QMenu>
#include <QKeyEvent>

#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    BsonTableView::BsonTableView(MongoShell *shell, const MongoQueryInfo &queryInfo, QWidget *parent) 
        :BaseClass(parent),_notifier(this,shell,queryInfo)
    {
#if defined(Q_OS_MAC)
        setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
        GuiRegistry::instance().setAlternatingColor(this);

        verticalHeader()->setDefaultAlignment(Qt::AlignLeft);
        horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
        //horizontalHeader()->setFixedHeight(25);   // commented because we shouldn't depend on heights in pixels - it may vary between platforms
        setStyleSheet("QTableView { border: none; gridline-color: #edebea;}");
        //setShowGrid(false);
        setSelectionMode(QAbstractItemView::ExtendedSelection);
        setSelectionBehavior(QAbstractItemView::SelectItems);
        setContextMenuPolicy(Qt::CustomContextMenu);
        VERIFY(connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&))));
    }

    void BsonTableView::keyPressEvent(QKeyEvent *event)
    {
        if (event->key()==Qt::Key_Delete){
            QModelIndexList indexses = selectionModel()->selectedRows();
            bool isForce = event->modifiers() & Qt::ShiftModifier;
            std::vector<BsonTreeItem*> items;
            for (QModelIndexList::const_iterator it = indexses.begin(); it!= indexses.end(); ++it)
            {
                BsonTreeItem *item = QtUtils::item<BsonTreeItem*>(*it);
                items.push_back(item);                
            }
            _notifier.deleteDocuments(items,isForce);
        }
        return BaseClass::keyPressEvent(event);
    }

    QModelIndex BsonTableView::selectedIndex() const
    {
        QModelIndexList indexses = selectionModel()->selectedIndexes();

        if (indexses.count() != 1)
            return QModelIndex();

        return indexses[0];
    }

    QModelIndexList BsonTableView::selectedIndexes() const
    {
        return selectionModel()->selectedRows();
    }

    void BsonTableView::showContextMenu( const QPoint &point )
    {
        QModelIndex selectedInd = selectedIndex();

        QPoint menuPoint = mapToGlobal(point);
        menuPoint.setY(menuPoint.y() + horizontalHeader()->height());
        menuPoint.setX(menuPoint.x() + verticalHeader()->width());

        if (selectedInd.isValid()){
            BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedInd);

            QMenu menu(this);
            _notifier.initMenu(&menu,documentItem);
            menu.exec(menuPoint);
        }else{
            QModelIndexList indexes = selectedIndexes();
            if(detail::isMultySelection(indexes)){
                QMenu menu(this);
                _notifier.initMultiSelectionMenu(&menu);
                menu.exec(menuPoint);
            }
        }
    }

}
