#include "robomongo/gui/widgets/workarea/BsonTableView.h"

#include <QHeaderView>
#include <QAction>
#include <QMenu>

#include "robomongo/gui/widgets/workarea/BsonTableItem.h"
#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    BsonTableView::BsonTableView(MongoShell *shell, const MongoQueryInfo &queryInfo, QWidget *parent) 
        :BaseClass(parent),
        _queryInfo(queryInfo)
    {
        verticalHeader()->setDefaultAlignment(Qt::AlignLeft);
        horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
        setStyleSheet("QTableView { border-left: 1px solid #c7c5c4; border-top: 1px solid #c7c5c4; }");
    }

    void BsonTableView::showContextMenu( const QPoint &a_point )
    {
        QModelIndexList indexses = selectionModel()->selectedRows();
        if (indexses.size()&&indexses.size()==1)
        {
            QModelIndex sourceIndex = indexses.at(0);
            if (sourceIndex.isValid()){                
                BsonTableItem *documentItem = static_cast<BsonTableItem *>(sourceIndex.internalPointer());
                if (documentItem){
                    
                }                
            }
        }
        
    }
}
