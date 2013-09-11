#include "robomongo/gui/widgets/workarea/BsonTableView.h"

#include <QHeaderView>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>

#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"

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
        setStyleSheet("QTableView { border-left: 1px solid #c7c5c4; border-top: 1px solid #c7c5c4; gridline-color: #edebea;}");
        //setShowGrid(false);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setContextMenuPolicy(Qt::CustomContextMenu);
        VERIFY(connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&))));
    }

    void BsonTableView::keyPressEvent(QKeyEvent *event)
    {
        if (event->key()==Qt::Key_Delete){
            QModelIndexList indexses = selectionModel()->selectedIndexes();
            for (QModelIndexList::const_iterator it = indexses.begin(); it!= indexses.end(); ++it)
            {
                BsonTreeItem *item = QtUtils::item<BsonTreeItem*>(*it);
                bool isForce = event->modifiers() & Qt::ShiftModifier;
                _notifier.deleteDocument(item,isForce);
            }
        }
        return BaseClass::keyPressEvent(event);
    }

    QModelIndex BsonTableView::selectedIndex() const
    {
        QModelIndexList indexses = selectionModel()->selectedIndexes();
        int count = indexses.count();

        if (indexses.count() != 1)
            return QModelIndex();

        return indexses[0];
    }

    void BsonTableView::showContextMenu( const QPoint &point )
    {
        QModelIndex selectedInd = selectedIndex();
        if (selectedInd.isValid()){
            BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedInd);
            documentItem = documentItem->childByKey(model()->headerData(selectedInd.column(),Qt::Horizontal,Qt::DisplayRole).toString());

            QMenu menu(this);
            _notifier.initMenu(&menu,documentItem);

            QPoint menuPoint = mapToGlobal(point);
            menuPoint.setY(menuPoint.y() + horizontalHeader()->height());
            menuPoint.setX(menuPoint.x() + verticalHeader()->width());
            menu.exec(menuPoint);
        }
    }

}
