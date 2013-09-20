#include "robomongo/gui/widgets/workarea/WorkAreaWidget.h"

#include <QHBoxLayout>

#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/domain/MongoShell.h"
#include "robomongo/core/utils/QtUtils.h"

#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/widgets/workarea/WorkAreaTabWidget.h"
#include "robomongo/gui/widgets/workarea/QueryWidget.h"

namespace Robomongo
{
    WorkAreaWidget::WorkAreaWidget(QWidget *parent)
        : QWidget(parent)
    {
        _tabWidget = new WorkAreaTabWidget(this);
        VERIFY(connect(_tabWidget, SIGNAL(currentChanged(int)),this, SIGNAL(tabActivated(int))));
        _tabWidget->setMovable(true);
        _tabWidget->setDocumentMode(true);

        QHBoxLayout *hlayout = new QHBoxLayout;
        hlayout->setContentsMargins(0, 3, 0, 0);
        hlayout->addWidget(_tabWidget);
        setLayout(hlayout);
    }

    void WorkAreaWidget::toggleOrientation()
    {
        QueryWidget *currentWidget = (QueryWidget *)_tabWidget->currentWidget();
        if (currentWidget)
            currentWidget->toggleOrientation();
    }

    void WorkAreaWidget::executeScript()
    {
        QueryWidget *currentWidget = (QueryWidget *)_tabWidget->currentWidget();
        if (currentWidget)
            currentWidget->execute();
    }

    void WorkAreaWidget::stopScript()
    {
        QueryWidget *currentWidget = (QueryWidget *)_tabWidget->currentWidget();
        if (currentWidget)
            currentWidget->stop();
    }

    void WorkAreaWidget::enterTextMode()
    {
        QueryWidget *currentWidget = (QueryWidget *)_tabWidget->currentWidget();
        if (currentWidget)
            currentWidget->enterTextMode();
    }

    void WorkAreaWidget::enterTreeMode()
    {
        QueryWidget *currentWidget = (QueryWidget *)_tabWidget->currentWidget();
        if (currentWidget)
            currentWidget->enterTreeMode();
    }

    void WorkAreaWidget::enterTableMode()
    {
        QueryWidget *currentWidget = (QueryWidget *)_tabWidget->currentWidget();
        if (currentWidget)
            currentWidget->enterTableMode();
    }

    void WorkAreaWidget::enterCustomMode()
    {
        QueryWidget *currentWidget = (QueryWidget *)_tabWidget->currentWidget();
        if (currentWidget)
            currentWidget->enterCustomMode();
    }

    int WorkAreaWidget::countTab() const
    {
        return _tabWidget->count();
    }

    QueryWidget *const WorkAreaWidget::currentWidget() const
    {
        return _tabWidget->currentQueryWidget();
    }

    void WorkAreaWidget::handle(OpeningShellEvent *event)
    {
        const QString &title = event->shell->title();

        QString shellName = title.isEmpty() ? " Loading..." : title;

        setUpdatesEnabled(false);
        QueryWidget *queryWidget = new QueryWidget(event->shell,_tabWidget,this);

        _tabWidget->addTab(queryWidget, shellName);
        _tabWidget->setCurrentIndex(_tabWidget->count() - 1);
#if !defined(Q_OS_MAC)
        _tabWidget->setTabIcon(_tabWidget->count() - 1, GuiRegistry::instance().mongodbIcon());
#endif
        setUpdatesEnabled(true);
        queryWidget->showProgress();
    }
}
