#include "WorkAreaTabWidget.h"
#include "WorkAreaWidget.h"
#include "WorkAreaTabBar.h"
#include "QueryWidget.h"
#include "AppRegistry.h"
#include "domain/App.h"

using namespace Robomongo;

/**
 * @brief Creates WorkAreaTabWidget.
 * @param workAreaWidget: WorkAreaWidget this tab belongs to.
 */
WorkAreaTabWidget::WorkAreaTabWidget(WorkAreaWidget * workAreaWidget) :
    QTabWidget(workAreaWidget)
{
    //setTabShape(QTabWidget::Triangular);
    //setStyleSheet("QTabWidget::pane { border-top: 2px solid red; }");
    // This line should go before setTabsClosable(true)
    setTabBar(new WorkAreaTabBar());



	setTabsClosable(true);
    setElideMode(Qt::ElideRight);

    connect(this, SIGNAL(tabCloseRequested(int)), SLOT(tabBar_tabCloseRequested(int)));
    connect(this, SIGNAL(currentChanged(int)), SLOT(ui_currentChanged(int)));

    connect(this->tabBar(), SIGNAL(newTabRequested(int)), SLOT(ui_newTabRequested(int)));
    connect(this->tabBar(), SIGNAL(reloadTabRequested(int)), SLOT(ui_reloadTabRequested(int)));
    connect(this->tabBar(), SIGNAL(duplicateTabRequested(int)), SLOT(ui_duplicateTabRequested(int)));
    connect(this->tabBar(), SIGNAL(closeOtherTabsRequested(int)), SLOT(ui_closeOtherTabsRequested(int)));
    connect(this->tabBar(), SIGNAL(closeTabsToTheRightRequested(int)), SLOT(ui_closeTabsToTheRightRequested(int)));
}

void WorkAreaTabWidget::closeTab(int index)
{
    if (index < 0)
        return;

    QueryWidget *tabWidget = (QueryWidget *) widget(index);
    removeTab(index);

    if (tabWidget)
    {
        AppRegistry::instance().app()->closeShell(tabWidget->shell());
        delete tabWidget;
    }
}

QueryWidget *WorkAreaTabWidget::currentQueryWidget()
{
    return static_cast<QueryWidget *>(currentWidget());
}

QueryWidget *WorkAreaTabWidget::queryWidget(int index)
{
    return static_cast<QueryWidget *>(widget(index));
}

/**
 * @brief Overrides QTabWidget::keyPressEvent() in order to intercept
 * tab close key shortcuts (Ctrl+F4 and Ctrl+W)
 */
void WorkAreaTabWidget::keyPressEvent(QKeyEvent *keyEvent)
{
    if ((keyEvent->modifiers() & Qt::ControlModifier) &&
        (keyEvent->key()==Qt::Key_F4 || keyEvent->key()==Qt::Key_W))
    {
        int index = currentIndex();
        removeTab(index);
    }

    QTabWidget::keyPressEvent(keyEvent);
}

void WorkAreaTabWidget::tabBar_tabCloseRequested(int index)
{
    closeTab(index);
}

void WorkAreaTabWidget::ui_newTabRequested(int index)
{
    queryWidget(index)->openNewTab();
}

void WorkAreaTabWidget::ui_reloadTabRequested(int index)
{
    QueryWidget *query = queryWidget(index);

    if (query)
        query->reload();
}

void WorkAreaTabWidget::ui_duplicateTabRequested(int index)
{
    QueryWidget *query = queryWidget(index);

    if (query)
        query->duplicate();
}

void WorkAreaTabWidget::ui_closeOtherTabsRequested(int index)
{
    tabBar()->moveTab(index, 0);
    while (count() > 1) {
        closeTab(1); // close second tab
    }
}

void WorkAreaTabWidget::ui_closeTabsToTheRightRequested(int index)
{
    while (count() > index + 1) {
        closeTab(index + 1); // close nearest tab
    }
}

void WorkAreaTabWidget::ui_currentChanged(int index)
{
    if (index < 0)
        return;

    QueryWidget *tabWidget = queryWidget(index);

    if (tabWidget)
        tabWidget->activateTabContent();
}
