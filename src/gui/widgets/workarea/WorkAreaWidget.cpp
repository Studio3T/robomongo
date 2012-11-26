#include "WorkAreaWidget.h"
#include "MainWindow.h"
#include "WorkAreaTabWidget.h"
#include "QueryWidget.h"
#include "mongodb/MongoCollection.h"
#include "AppRegistry.h"
#include "GuiRegistry.h"

using namespace Robomongo;

/*
** Constructs work area
*/
WorkAreaWidget::WorkAreaWidget(MainWindow * mainWindow)	: QWidget(mainWindow)
{
	_mainWindow = mainWindow;
    _tabWidget = new WorkAreaTabWidget(this);
	_tabWidget->setMovable(true);

	QHBoxLayout * hlayout = new QHBoxLayout;
	hlayout->setMargin(0);
	hlayout->addWidget(_tabWidget);
	setLayout(hlayout);

    //connect(_viewModel, SIGNAL(queryWindowAdded(QueryWindowViewModel *)), SLOT(vm_queryWindowAdded(QueryWindowViewModel *)));
}

void WorkAreaWidget::vm_queryWindowAdded()
{
    QLabel * queryWidget = new QLabel("Hello");
//	QueryWidget * queryWidget = new QueryWidget(viewModel, this);
    _tabWidget->addTab(queryWidget, "Robotab" /* viewModel->title()*/);
	_tabWidget->setCurrentIndex(_tabWidget->count() - 1);

    _tabWidget->setTabIcon(_tabWidget->count() - 1, GuiRegistry::instance().collectionIcon());
	
}
