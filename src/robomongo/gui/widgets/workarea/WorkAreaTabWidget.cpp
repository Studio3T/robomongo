#include "robomongo/gui/widgets/workarea/WorkAreaTabWidget.h"

#include <QKeyEvent>

#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/KeyboardManager.h"
#include "robomongo/core/domain/MongoShell.h"

#include "robomongo/gui/widgets/workarea/WorkAreaTabBar.h"
#include "robomongo/gui/widgets/workarea/QueryWidget.h"
#include "robomongo/gui/GuiRegistry.h"

namespace Robomongo
{

    /**
     * @brief Creates WorkAreaTabWidget.
     * @param workAreaWidget: WorkAreaWidget this tab belongs to.
     */
    WorkAreaTabWidget::WorkAreaTabWidget(QWidget *parent) :
        QTabWidget(parent)
    {
        WorkAreaTabBar *tab = new WorkAreaTabBar(this);
        // This line (setTabBar()) should go before setTabsClosable(true)
        setTabBar(tab);
        setTabsClosable(true);
        setElideMode(Qt::ElideRight);
        setMovable(true);
        setDocumentMode(true);
        setTabBarAutoHide(true);
        VERIFY(connect(this, SIGNAL(tabCloseRequested(int)), SLOT(tabBar_tabCloseRequested(int))));
        VERIFY(connect(this, SIGNAL(currentChanged(int)), SLOT(ui_currentChanged(int))));

        VERIFY(connect(tab, SIGNAL(newTabRequested(int)), SLOT(ui_newTabRequested(int))));
        VERIFY(connect(tab, SIGNAL(reloadTabRequested(int)), SLOT(ui_reloadTabRequested(int))));
        VERIFY(connect(tab, SIGNAL(duplicateTabRequested(int)), SLOT(ui_duplicateTabRequested(int))));
        VERIFY(connect(tab, SIGNAL(closeOtherTabsRequested(int)), SLOT(ui_closeOtherTabsRequested(int))));
        VERIFY(connect(tab, SIGNAL(closeTabsToTheRightRequested(int)), SLOT(ui_closeTabsToTheRightRequested(int))));
    }

    void WorkAreaTabWidget::closeTab(int index)
    {
        if (index >= 0)
        {
            QueryWidget *tabWidget = queryWidget(index);
            removeTab(index);
            delete tabWidget;
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
        return qobject_cast<QueryWidget *>(currentWidget());
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
        QueryWidget *widget = currentQueryWidget();

        if (KeyboardManager::isPreviousTabShortcut(keyEvent)) {
            previousTab();
            return;
        } else if (KeyboardManager::isNextTabShortcut(keyEvent)) {
            nextTab();
            return;
        } else if (KeyboardManager::isNewTabShortcut(keyEvent) && widget) {
            widget->openNewTab();
            return;
        } else if (KeyboardManager::isDuplicateTabShortcut(keyEvent) && widget) {
            widget->duplicate();
            return;
        } else if (KeyboardManager::isSetFocusOnQueryLineShortcut(keyEvent) && widget) {
            widget->setScriptFocus();
            return;
        } else if (KeyboardManager::isExecuteScriptShortcut(keyEvent) && widget) {
            widget->execute();
            return;
        } else if (KeyboardManager::isAutoCompleteShortcut(keyEvent) && widget) {
            widget->showAutocompletion();
            return;
        } else if (KeyboardManager::isHideAutoCompleteShortcut(keyEvent) && widget) {
            widget->hideAutocompletion();
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
        if (index < 0)
            return;

        QueryWidget *tabWidget = queryWidget(index);

        if (tabWidget)
            tabWidget->activateTabContent();
    }

    void WorkAreaTabWidget::tabTextChange(const QString &text)
    {
        QWidget *send = qobject_cast<QWidget*>(sender());
        if(!send)
            return;

        setTabText(indexOf(send), text);
    }

    void WorkAreaTabWidget::tooltipTextChange(const QString &text)
    {
        QWidget *send = qobject_cast<QWidget*>(sender());
        if(!send)
            return;

        setTabToolTip(indexOf(send), text);
    }

    void WorkAreaTabWidget::handle(OpeningShellEvent *event)
    {
        const QString &title = event->shell->title();

        QString shellName = title.isEmpty() ? " Loading..." : title;

        QueryWidget *queryWidget = new QueryWidget(event->shell,this);
        VERIFY(connect(queryWidget, SIGNAL(titleChanged(const QString &)), this, SLOT(tabTextChange(const QString &))));
        VERIFY(connect(queryWidget, SIGNAL(toolTipChanged(const QString &)), this, SLOT(tooltipTextChange(const QString &))));

        addTab(queryWidget, shellName);

        setCurrentIndex(count() - 1);
        if (!AppRegistry::instance().settingsManager()->useTabbar()) {
            ui_closeOtherTabsRequested(currentIndex());
        }

#if !defined(Q_OS_MAC)
        setTabIcon(count() - 1, GuiRegistry::instance().mongodbIcon());
#endif
        if (event->shell->isExecutable()) {
            queryWidget->showProgress();
        }
    }
}

