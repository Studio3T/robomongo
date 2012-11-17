#include <QtGui>
#include "MainWindow.h"
#include "GuiRegistry.h"
#include "AppRegistry.h"
#include "dialogs/ConnectionsDialog.h"
#include "settings/SettingsManager.h"
#include "widgets/LogWidget.h"
#include "widgets/explorer/ExplorerWidget.h"

using namespace Robomongo;

MainWindow::MainWindow() : QMainWindow()
{
    // Exit action
    QAction *exitAction = new QAction("Exit", this);
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

    // Connect action
    QAction *connectAction = new QAction("Connect", this);
    connectAction->setIcon(GuiRegistry::instance().serverIcon());
    connectAction->setIconText("Connect");
    connect(connectAction, SIGNAL(triggered()), this, SLOT(manageConnections()));

    // Refresh action
    QAction *refreshAction = new QAction("Refresh", this);
    refreshAction->setIcon(qApp->style()->standardIcon(QStyle::SP_BrowserReload));

    // File menu
    QMenu *fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction(connectAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    // Toolbar
    QToolBar *toolBar = new QToolBar("Toolbar", this);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolBar->addAction(connectAction);
    toolBar->addAction(refreshAction);
    toolBar->addSeparator();
    addToolBar(toolBar);

    _status = new QLabel;
    statusBar()->addPermanentWidget(_status);

    //createTabs();
    createDatabaseExplorer();

    setWindowTitle("Robomongo 0.2");
    setWindowIcon(GuiRegistry::instance().databaseIcon());

    setCentralWidget(new QWidget(this));

    //connect(_viewModel, SIGNAL(statusMessageUpdated(QString)), SLOT(vm_statusMessageUpdated(QString)));
}

void MainWindow::manageConnections()
{
    ConnectionsDialog dialog(&AppRegistry::instance().settingsManager());
    dialog.exec();

    AppRegistry::instance().settingsManager().save();
}

void MainWindow::createDatabaseExplorer()
{
    /*
    ** Explorer
    */
    _explorer = new ExplorerWidget(this);
    QDockWidget *explorerDock = new QDockWidget(tr(" Database Explorer"));
    explorerDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    explorerDock->setWidget(_explorer);
    addDockWidget(Qt::LeftDockWidgetArea, explorerDock);

    _log = new LogWidget(this);
    QDockWidget *logDock = new QDockWidget(tr(" Log"));
    logDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    logDock->setWidget(_log);
    addDockWidget(Qt::BottomDockWidgetArea, logDock);
}
