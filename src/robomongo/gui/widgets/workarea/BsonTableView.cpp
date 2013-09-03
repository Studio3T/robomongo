#include "robomongo/gui/widgets/workarea/BsonTableView.h"

#include <QHeaderView>

namespace Robomongo
{
    BsonTableView::BsonTableView(MongoShell *shell, QWidget *parent) 
        :BaseClass(parent)
    {
        verticalHeader()->setDefaultAlignment(Qt::AlignLeft);
        horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
        setStyleSheet("QTableView { border-left: 1px solid #c7c5c4; border-top: 1px solid #c7c5c4; }");
    }
}
