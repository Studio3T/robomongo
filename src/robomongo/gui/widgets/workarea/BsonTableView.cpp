#include "robomongo/gui/widgets/workarea/BsonTableView.h"

#include <QHeaderView>

namespace Robomongo
{
    BsonTableView::BsonTableView(MongoShell *shell, QWidget *parent) 
        :BaseClass(parent)
    {
        verticalHeader()->setDefaultAlignment(Qt::AlignLeft);
        horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    }
}
