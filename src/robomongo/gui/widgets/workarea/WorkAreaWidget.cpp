#include "robomongo/gui/widgets/workarea/WorkAreaWidget.h"

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/gui/MainWindow.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/widgets/workarea/WorkAreaTabWidget.h"
#include "robomongo/gui/widgets/workarea/QueryWidget.h"

using namespace Robomongo;

WorkAreaWidget::WorkAreaWidget(MainWindow *mainWindow)	:
    QWidget(),
    _bus(AppRegistry::instance().bus())
{
	_mainWindow = mainWindow;
    _tabWidget = new WorkAreaTabWidget(this);
	_tabWidget->setMovable(true);
    _tabWidget->setDocumentMode(true);

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->setContentsMargins(0, 3, 0, 0);
	hlayout->addWidget(_tabWidget);
	setLayout(hlayout);

    _bus->subscribe(this, OpeningShellEvent::Type);
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

void WorkAreaWidget::enterCustomMode()
{
    QueryWidget *currentWidget = (QueryWidget *)_tabWidget->currentWidget();
    if (currentWidget)
        currentWidget->enterCustomMode();
}

void WorkAreaWidget::handle(OpeningShellEvent *event)
{
    ScriptInfo &info = event->scriptInfo;

    QString shellName = info.title().isEmpty() ? " Loading..." : info.title();

    setUpdatesEnabled(false);
    QueryWidget *queryWidget = new QueryWidget(
        event->shell,
        _tabWidget,
        info,
        _mainWindow->viewMode());

    _tabWidget->addTab(queryWidget, shellName);
    _tabWidget->setCurrentIndex(_tabWidget->count() - 1);
    _tabWidget->setTabIcon(_tabWidget->count() - 1, GuiRegistry::instance().mongodbIcon());
    setUpdatesEnabled(true);
    queryWidget->showProgress();
}
