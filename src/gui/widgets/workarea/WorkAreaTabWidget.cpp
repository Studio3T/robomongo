#include "WorkAreaTabWidget.h"
#include "WorkAreaWidget.h"
#include "WorkAreaTabBar.h"
#include "QueryWidget.h"

using namespace Robomongo;

WorkAreaTabWidget::WorkAreaTabWidget(WorkAreaWidget * workAreaWidget) : QTabWidget(workAreaWidget)
{
    setTabBar(new WorkAreaTabBar());  // Should go before setTabsClosable(true)!
	setTabsClosable(true);
    setElideMode(Qt::ElideRight);
	connect(this, SIGNAL(tabCloseRequested(int)), SLOT(ui_tabCloseRequested(int)));
    connect(this, SIGNAL(currentChanged(int)), SLOT(ui_currentChanged(int)));
}

void WorkAreaTabWidget::ui_tabCloseRequested(int index)
{
	if (index >= 0)
	{
		QWidget * tabWidget = widget(index);
		removeTab(index);

		if (tabWidget)
			delete tabWidget;		
    }
}

void WorkAreaTabWidget::ui_currentChanged(int index)
{
    if (!index)
        return;

    QueryWidget *tabWidget = (QueryWidget *) widget(index);

    if (tabWidget)
        tabWidget->activateTabContent();
}
