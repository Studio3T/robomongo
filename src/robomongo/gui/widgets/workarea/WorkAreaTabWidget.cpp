#include "robomongo/gui/widgets/workarea/WorkAreaTabWidget.h"

#include "robomongo/gui/widgets/workarea/WorkAreaWidget.h"
#include "robomongo/gui/widgets/workarea/WorkAreaTabBar.h"
#include "robomongo/gui/widgets/workarea/QueryWidget.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/domain/MongoShell.h"
#include "robomongo/core/EventBus.h"

namespace Robomongo
{

    /**
     * @brief Creates WorkAreaTabWidget.
     * @param workAreaWidget: WorkAreaWidget this tab belongs to.
     */
    WorkAreaTabWidget::WorkAreaTabWidget(WorkAreaWidget *workAreaWidget) :
        QTabWidget(workAreaWidget),
        _bus(AppRegistry::instance().bus())
    {
        // This line (setTabBar()) should go before setTabsClosable(true)
        WorkAreaTabBar * tab = new WorkAreaTabBar(this);
        setTabBar(tab);
        setTabsClosable(true);
        setElideMode(Qt::ElideRight);

        connect(this, SIGNAL(tabCloseRequested(int)), SLOT(tabBar_tabCloseRequested(int)));
        connect(this, SIGNAL(currentChanged(int)), SLOT(ui_currentChanged(int)));

        connect(tab, SIGNAL(newTabRequested(int)), SLOT(ui_newTabRequested(int)));
        connect(tab, SIGNAL(reloadTabRequested(int)), SLOT(ui_reloadTabRequested(int)));
        connect(tab, SIGNAL(duplicateTabRequested(int)), SLOT(ui_duplicateTabRequested(int)));
        connect(tab, SIGNAL(closeOtherTabsRequested(int)), SLOT(ui_closeOtherTabsRequested(int)));
        connect(tab, SIGNAL(closeTabsToTheRightRequested(int)), SLOT(ui_closeTabsToTheRightRequested(int)));

        connect(tab, SIGNAL(openTabFile(int)), SLOT(openedTabFile(int)));
        connect(tab, SIGNAL(saveTabToFile(int)), SLOT(savedTabToFile(int)));
        connect(tab, SIGNAL(saveTabToFileAs(int)), SLOT(savedTabToFileAs(int)));
    }

    void WorkAreaTabWidget::closeTab(int index)
    {
        if (index >= 0)
        {
            QueryWidget *tabWidget = queryWidget(index);
            removeTab(index);
            if (tabWidget)
            {
                AppRegistry::instance().app()->closeShell(tabWidget->shell());
                delete tabWidget;
            }
        }
    }

    void WorkAreaTabWidget::nextTab()
    {
        int index = currentIndex();
        int tabsCount = count();
        if (index == tabsCount - 1)
        {
            setCurrentIndex(0);
            return;
        }
        if (index >= 0 && index < tabsCount - 1)
        {
            setCurrentIndex(index + 1);
            return;
        }
    }

    void WorkAreaTabWidget::previousTab()
    {
        int index = currentIndex();
        if (index == 0)
        {
            setCurrentIndex(count() - 1);
            return;
        }
        if (index > 0)
        {
            setCurrentIndex(index - 1);
            return;
        }
    }

    QueryWidget *WorkAreaTabWidget::currentQueryWidget()
    {
        return static_cast<QueryWidget *>(currentWidget());
    }

    QueryWidget *WorkAreaTabWidget::queryWidget(int index)
    {
        return qobject_cast<QueryWidget *>(widget(index));
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
            closeTab(index);
            return;
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
        if (index == -1) {
            _bus->publish(new AllTabsClosedEvent(this));
        }

        if (index < 0)
            return;

        QueryWidget *tabWidget = queryWidget(index);

        if (tabWidget)
            tabWidget->activateTabContent();
    }
    void WorkAreaTabWidget::savedTabToFile(int index)
    {
        QueryWidget *query = queryWidget(index);
        if (query){
            MongoShell * shell = query->shell();
            if(shell){
                shell->saveToFile();
            }
        }
    }
    void WorkAreaTabWidget::savedTabToFileAs(int index)
    {
        QueryWidget *query = queryWidget(index);
        if (query){
            MongoShell * shell = query->shell();
            if(shell){
                shell->saveToFileAs();
            }
        }
    }
    void WorkAreaTabWidget::openedTabFile(int index)
    {
        QueryWidget *query = queryWidget(index);
        if (query){
            MongoShell * shell = query->shell();
            if(shell){
                shell->loadFromFile();
            }
        }
    }
}

