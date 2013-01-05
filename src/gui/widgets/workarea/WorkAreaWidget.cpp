#include "WorkAreaWidget.h"
#include "MainWindow.h"
#include "WorkAreaTabWidget.h"
#include "QueryWidget.h"
#include "domain/MongoCollection.h"
#include "AppRegistry.h"
#include "GuiRegistry.h"
#include "EventBus.h"

using namespace Robomongo;

/*
** Constructs work area
*/
WorkAreaWidget::WorkAreaWidget(MainWindow * mainWindow)	:
    QWidget(),
    _bus(AppRegistry::instance().bus())
{
	_mainWindow = mainWindow;
    _tabWidget = new WorkAreaTabWidget(this);
	_tabWidget->setMovable(true);
    _tabWidget->setDocumentMode(true);

	QHBoxLayout * hlayout = new QHBoxLayout;
    hlayout->setContentsMargins(0, 3, 0, 0);
	hlayout->addWidget(_tabWidget);
	setLayout(hlayout);

    _bus->subscribe(this, OpeningShellEvent::Type);
}

WorkAreaWidget::~WorkAreaWidget() { }

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
        currentWidget->ui_executeButtonClicked();
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

void WorkAreaWidget::handle(OpeningShellEvent *event)
{
    bool textMode = _mainWindow->textMode();
    QString shellName = event->shellName.isEmpty() ? "Loading..." : event->shellName;

    setUpdatesEnabled(false);
    QueryWidget * queryWidget = new QueryWidget(event->shell, _tabWidget, event->initialScript, textMode);
    _tabWidget->addTab(queryWidget, shellName);
    _tabWidget->setCurrentIndex(_tabWidget->count() - 1);
    _tabWidget->setTabIcon(_tabWidget->count() - 1, GuiRegistry::instance().mongodbIcon());
    setUpdatesEnabled(true);
}
