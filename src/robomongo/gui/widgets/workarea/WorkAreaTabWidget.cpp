#include "robomongo/gui/widgets/workarea/WorkAreaTabWidget.h"

#include <QKeyEvent>
#include <QScrollArea>

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/KeyboardManager.h"
#include "robomongo/core/domain/MongoShell.h"

#include "robomongo/gui/widgets/workarea/WorkAreaTabBar.h"
#include "robomongo/gui/widgets/workarea/QueryWidget.h"
#include "robomongo/gui/widgets/workarea/WelcomeTab.h"
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
        auto tab = new WorkAreaTabBar(this);
        // This line (setTabBar()) should go before setTabsClosable(true)
        setTabBar(tab);
        setTabsClosable(true);
        setElideMode(Qt::ElideRight);
        setMovable(true);
        setDocumentMode(true);

#ifdef Q_OS_MAC
        setDocumentMode(false);
        QFont font = tab->font();
        font.setPixelSize(12);
        tab->setFont(font);
        QString styles = QString(
            "QTabWidget::pane { background-color: white; }"   // This style disables default styling under Mac
            "QTabWidget::tab-bar {"
                "alignment: left;"
            "}"
            "QTabBar::tab:selected { "
                "background: white; /*#E1E1E1*/; "
                "color: #282828;"
            "} "
            "QTabBar::tab {"
                "color: #505050;"
                "font-size: 11px;"
                "background: %1;"
                "border-right: 1px solid #aaaaaa;"
                "padding: 4px 5px 7px 5px;"
            "}"
        ).arg(QWidget::palette().color(QWidget::backgroundRole()).darker(114).name());
        setStyleSheet(styles);
#endif

        VERIFY(connect(this, SIGNAL(tabCloseRequested(int)), SLOT(tabBar_tabCloseRequested(int))));
        VERIFY(connect(this, SIGNAL(currentChanged(int)), SLOT(ui_currentChanged(int))));

        VERIFY(connect(tab, SIGNAL(newTabRequested(int)), SLOT(ui_newTabRequested(int))));
        VERIFY(connect(tab, SIGNAL(reloadTabRequested(int)), SLOT(ui_reloadTabRequested(int))));
        VERIFY(connect(tab, SIGNAL(duplicateTabRequested(int)), SLOT(ui_duplicateTabRequested(int))));
        VERIFY(connect(tab, SIGNAL(closeOtherTabsRequested(int)), SLOT(ui_closeOtherTabsRequested(int))));
        VERIFY(connect(tab, SIGNAL(closeTabsToTheRightRequested(int)), SLOT(ui_closeTabsToTheRightRequested(int))));

        auto scrollArea = new QScrollArea;
        _welcomeTab = new WelcomeTab(scrollArea);
        scrollArea->setWidget(_welcomeTab);
        scrollArea->setBackgroundRole(QPalette::Base);

#ifdef __APPLE__
        addTab(scrollArea, QIcon(), "Welcome");
#else
        addTab(scrollArea, GuiRegistry::instance().welcomeTabIcon(), "Welcome");
#endif
        scrollArea->setFrameShape(QFrame::NoFrame);
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
        return qobject_cast<QueryWidget*>(widget(index));
    }

    WelcomeTab* WorkAreaTabWidget::getWelcomeTab()
    {
        return _welcomeTab;
    }

    void WorkAreaTabWidget::openWelcomeTab()
    {
        auto scrollArea = qobject_cast<QScrollArea*>(_welcomeTab->getParent());
        if (!scrollArea)
            return;

        _welcomeTab = new WelcomeTab(scrollArea);
        scrollArea->setWidget(_welcomeTab);
        scrollArea->setBackgroundRole(QPalette::Base);

#ifdef __APPLE__
        QIcon icon;
#else
        QIcon const& icon = GuiRegistry::instance().welcomeTabIcon();
#endif
        // If welcome tab is closed open it as first tab otherwise refresh on 
        // it's current place.
        if (indexOf(scrollArea) == -1)  // Welcome Tab is closed
            insertTab(0, scrollArea, icon, "Welcome");
        else 
            insertTab(indexOf(scrollArea), scrollArea, icon, "Welcome");

        scrollArea->setFrameShape(QFrame::NoFrame);
        setCurrentIndex(indexOf(scrollArea));
    }

    /**
     * @brief Overrides QTabWidget::keyPressEvent() in order to intercept
     * tab close key shortcuts (Ctrl+F4 and Ctrl+W)
     */
    void WorkAreaTabWidget::keyPressEvent(QKeyEvent *keyEvent)
    {
        if ((keyEvent->modifiers() & Qt::ControlModifier) &&
            (keyEvent->key() == Qt::Key_F4 || keyEvent->key() == Qt::Key_W))
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

    void WorkAreaTabWidget::resizeEvent(QResizeEvent* event)
    {
        QTabWidget::resizeEvent(event);

        if (_welcomeTab->isVisible())
            _welcomeTab->resize();
    }

    void WorkAreaTabWidget::tabBar_tabCloseRequested(int index)
    {
        closeTab(index);
    }

    void WorkAreaTabWidget::ui_newTabRequested(int index)
    {
        if (QueryWidget *query = queryWidget(index))
            query->openNewTab();
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
        if (!send)
            return;

        setTabText(indexOf(send), text);        
    }

    void WorkAreaTabWidget::tooltipTextChange(const QString &text)
    {
        QWidget *send = qobject_cast<QWidget*>(sender());
        if (!send)
            return;

        setTabToolTip(indexOf(send), text);
    }

    void WorkAreaTabWidget::handle(OpeningShellEvent *event)
    {
        const QString &title = event->shell->title();

        QString shellName = title.isEmpty() ? " Loading..." : title;

        auto queryWidget = new QueryWidget(event->shell, this);
        VERIFY(connect(queryWidget, SIGNAL(titleChanged(const QString &)), this, SLOT(tabTextChange(const QString &))));
        VERIFY(connect(queryWidget, SIGNAL(toolTipChanged(const QString &)), this, SLOT(tooltipTextChange(const QString &))));
        
        addTab(queryWidget, shellName);

        setCurrentIndex(count() - 1);
#if !defined(Q_OS_MAC)
        setTabIcon(count() - 1, GuiRegistry::instance().mongodbIcon());
#endif
        queryWidget->showProgress();
    }
}

