#include "WorkAreaTabWidget.h"
#include "WorkAreaWidget.h"
#include "WorkAreaTabBar.h"

using namespace Robomongo;

WorkAreaTabWidget::WorkAreaTabWidget(WorkAreaWidget * workAreaWidget) : QTabWidget(workAreaWidget)
{
    setTabBar(new WorkAreaTabBar());  // Should go before setTabsClosable(true)!
	setTabsClosable(true);
	connect(this, SIGNAL(tabCloseRequested(int)), SLOT(ui_tabCloseRequested(int)));
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
