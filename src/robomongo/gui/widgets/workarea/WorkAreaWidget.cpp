#include "robomongo/gui/widgets/workarea/WorkAreaWidget.h"

#include <QHBoxLayout>

#include "robomongo/gui/widgets/workarea/WorkAreaTabWidget.h"

namespace Robomongo
{
    WorkAreaWidget::WorkAreaWidget(WorkAreaTabWidget *tabWidget) :
        _tabWidget(tabWidget)
    {
        QHBoxLayout *hlayout = new QHBoxLayout;
        hlayout->setContentsMargins(0, 3, 0, 0);
        hlayout->addWidget(_tabWidget);
        setLayout(hlayout);
    }
}
