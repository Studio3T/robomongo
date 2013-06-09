#include "robomongo/gui/MainWindow.h"

#include <QtGui>
#include <QMessageBox>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/mongodb/MongoException.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/dialogs/ConnectionsDialog.h"
#include "robomongo/gui/widgets/LogWidget.h"
#include "robomongo/gui/widgets/explorer/ExplorerWidget.h"
#include "robomongo/gui/widgets/workarea/WorkAreaWidget.h"
#include "robomongo/gui/dialogs/DocumentTextEditor.h"
#include "robomongo/gui/dialogs/AboutDialog.h"

using namespace Robomongo;

MainWindow::MainWindow() : QMainWindow(),
    _app(AppRegistry::instance().app()),
    _settingsManager(AppRegistry::instance().settingsManager()),
    _bus(AppRegistry::instance().bus()),
    _workArea(NULL),
    _connectionsMenu(NULL),
    _viewMode(Text)
{
    GuiRegistry::instance().setMainWindow(this);

    _bus->subscribe(this, ConnectionFailedEvent::Type);
    _bus->subscribe(this, ScriptExecutedEvent::Type);
    _bus->subscribe(this, ScriptExecutingEvent::Type);
    _bus->subscribe(this, QueryWidgetUpdatedEvent::Type);
    _bus->subscribe(this, AllTabsClosedEvent::Type);

    QColor background = palette().window().color();

#if defined(Q_OS_MAC)
    QString explorerColor = "#DEE3EA"; // was #CED6DF"
#elif defined(Q_OS_LINUX)
    QString explorerColor = background.darker(103).name();
#else
    QString explorerColor = background.lighter(103).name();
#endif

    qApp->setStyleSheet(QString(
        "Robomongo--ExplorerTreeWidget#explorerTree { padding: 1px 0px 0px 0px; background-color: %1; border: 0px; } \n " // #E7E5E4
        "QWidget#queryWidget { background-color:#E7E5E4; margin: 0px; padding:0px; } "
        "QMainWindow::separator { background: #E7E5E4; width: 1px; }"
    ).arg(explorerColor));

    // Exit action
    QAction *exitAction = new QAction("&Exit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

    // Connect action
    _connectAction = new QAction("&Connect", this);
    _connectAction->setShortcut(QKeySequence::Open);
    _connectAction->setIcon(GuiRegistry::instance().connectIcon());
    _connectAction->setIconText("Connect");
    _connectAction->setToolTip("Connect to local or remote MongoDB instance <b>(Ctrl + O)</b>");
    connect(_connectAction, SIGNAL(triggered()), this, SLOT(manageConnections()));

    _connectionsMenu = new ConnectionMenu(this);
    connect(_connectionsMenu, SIGNAL(triggered(QAction*)), this, SLOT(connectToServer(QAction*)));
    updateConnectionsMenu();

    QToolButton *connectButton = new QToolButton();
    connectButton->setText("&Connect");
    connectButton->setIcon(GuiRegistry::instance().connectIcon());
    connectButton->setFocusPolicy(Qt::NoFocus);
    connectButton->setToolTip("Connect to local or remote MongoDB instance <b>(Ctrl + O)</b>");
    connectButton->setToolButtonStyle(Qt::ToolButtonIconOnly);

#if !defined(Q_OS_MAC)
    connectButton->setMenu(_connectionsMenu);
    connectButton->setPopupMode(QToolButton::MenuButtonPopup);
#endif

    connect(connectButton, SIGNAL(clicked()), this, SLOT(manageConnections()));

    QWidgetAction*connectButtonAction = new QWidgetAction(this);
    connectButtonAction->setDefaultWidget(connectButton);

    // Orientation action
    _orientationAction = new QAction("&Rotate", this);
    _orientationAction->setShortcut(Qt::Key_F10);
    _orientationAction->setIcon(GuiRegistry::instance().rotateIcon());
    _orientationAction->setToolTip("Toggle orientation of results view <b>(F10)</b>");
    connect(_orientationAction, SIGNAL(triggered()), this, SLOT(toggleOrientation()));

    // read view mode setting
    _viewMode = _settingsManager->viewMode();

    // Text mode action
    QAction *textModeAction = new QAction("&Text Mode", this);
    textModeAction->setShortcut(Qt::Key_F4);
    textModeAction->setIcon(GuiRegistry::instance().textHighlightedIcon());
    textModeAction->setToolTip("Show current tab in text mode, and make this mode default for all subsequent queries <b>(F4)</b>");
    textModeAction->setCheckable(true);
    textModeAction->setChecked(_viewMode == Text);
    connect(textModeAction, SIGNAL(triggered()), this, SLOT(enterTextMode()));

    // Tree mode action
    QAction *treeModeAction = new QAction("&Tree Mode", this);
    treeModeAction->setShortcut(Qt::Key_F3);
    treeModeAction->setIcon(GuiRegistry::instance().treeHighlightedIcon());
    treeModeAction->setToolTip("Show current tab in tree mode, and make this mode default for all subsequent queries <b>(F3)</b>");
    treeModeAction->setCheckable(true);
    treeModeAction->setChecked(_viewMode == Tree);
    connect(treeModeAction, SIGNAL(triggered()), this, SLOT(enterTreeMode()));

    // Custom mode action
    QAction *customModeAction = new QAction("&Custom Mode", this);
    customModeAction->setShortcut(Qt::Key_F2);
    customModeAction->setIcon(GuiRegistry::instance().customHighlightedIcon());
    customModeAction->setToolTip("Show current tab in custom mode if possible, and make this mode default for all subsequent queries <b>(F2)</b>");
    customModeAction->setCheckable(true);
    customModeAction->setChecked(_viewMode == Custom);
    connect(customModeAction, SIGNAL(triggered()), this, SLOT(enterCustomMode()));

    QActionGroup *modeGroup = new QActionGroup(this);
    modeGroup->addAction(textModeAction);
    modeGroup->addAction(treeModeAction);
    modeGroup->addAction(customModeAction);

    // Execute action
    _executeAction = new QAction("", this);
    _executeAction->setData("Execute");
    _executeAction->setIcon(GuiRegistry::instance().executeIcon());
    _executeAction->setShortcut(Qt::Key_F5);
    _executeAction->setToolTip("Execute query for current tab. If you have some selection in query text - only selection will be executed <b>(F5 </b> or <b>Ctrl + Enter)</b>");
    connect(_executeAction, SIGNAL(triggered()), SLOT(executeScript()));

    // Stop action
    _stopAction = new QAction("", this);
    _stopAction->setData("Stop");
    _stopAction->setIcon(GuiRegistry::instance().stopIcon());
    _stopAction->setShortcut(Qt::Key_F6);
    _stopAction->setToolTip("Stop execution of currently running script. <b>(F6)</b>");
    _stopAction->setDisabled(true);
    connect(_stopAction, SIGNAL(triggered()), SLOT(stopScript()));

    // Full screen action
    QAction *fullScreenAction = new QAction("&Full Screen", this);
    fullScreenAction->setShortcut(Qt::Key_F11);
    fullScreenAction->setVisible(true);
    connect(fullScreenAction, SIGNAL(triggered()), this, SLOT(toggleFullScreen2()));
    fullScreenAction->setVisible(false);

    // Refresh action
    QAction *refreshAction = new QAction("Refresh", this);
    refreshAction->setIcon(qApp->style()->standardIcon(QStyle::SP_BrowserReload));
    connect(refreshAction, SIGNAL(triggered()), this, SLOT(refreshConnections()));

    // File menu
    QMenu *fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction(_connectAction);
    fileMenu->addAction(fullScreenAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    // Options menu
    QMenu *optionsMenu = menuBar()->addMenu("Options");
    optionsMenu->addAction(customModeAction);
    optionsMenu->addAction(treeModeAction);
    optionsMenu->addAction(textModeAction);
    optionsMenu->addSeparator();

    // UUID encoding
    QAction *defaultEncodingAction = new QAction("Do not decode (show as is)", this);
    defaultEncodingAction->setCheckable(true);
    defaultEncodingAction->setChecked(_settingsManager->uuidEncoding() == DefaultEncoding);
    connect(defaultEncodingAction, SIGNAL(triggered()), this, SLOT(setDefaultUuidEncoding()));

    QAction *javaLegacyEncodingAction = new QAction("Use Java Encoding", this);
    javaLegacyEncodingAction->setCheckable(true);
    javaLegacyEncodingAction->setChecked(_settingsManager->uuidEncoding() == JavaLegacy);
    connect(javaLegacyEncodingAction, SIGNAL(triggered()), this, SLOT(setJavaUuidEncoding()));

    QAction *csharpLegacyEncodingAction = new QAction("Use .NET Encoding", this);
    csharpLegacyEncodingAction->setCheckable(true);
    csharpLegacyEncodingAction->setChecked(_settingsManager->uuidEncoding() == CSharpLegacy);
    connect(csharpLegacyEncodingAction, SIGNAL(triggered()), this, SLOT(setCSharpUuidEncoding()));

    QAction *pythonEncodingAction = new QAction("Use Python Encoding", this);
    pythonEncodingAction->setCheckable(true);
    pythonEncodingAction->setChecked(_settingsManager->uuidEncoding() == PythonLegacy);
    connect(pythonEncodingAction, SIGNAL(triggered()), this, SLOT(setPythonUuidEncoding()));

    QMenu *uuidMenu = optionsMenu->addMenu("Legacy UUID Encoding");
    uuidMenu->addAction(defaultEncodingAction);
    uuidMenu->addAction(javaLegacyEncodingAction);
    uuidMenu->addAction(csharpLegacyEncodingAction);
    uuidMenu->addAction(pythonEncodingAction);

    QActionGroup *uuidEncodingGroup = new QActionGroup(this);
    uuidEncodingGroup->addAction(defaultEncodingAction);
    uuidEncodingGroup->addAction(javaLegacyEncodingAction);
    uuidEncodingGroup->addAction(csharpLegacyEncodingAction);
    uuidEncodingGroup->addAction(pythonEncodingAction);

    QAction *aboutRobomongoAction = new QAction("&About Robomongo", this);
    connect(aboutRobomongoAction, SIGNAL(triggered()), this, SLOT(aboutRobomongo()));

    // Options menu
    QMenu *helpMenu = menuBar()->addMenu("Help");
    helpMenu->addAction(aboutRobomongoAction);

    // Toolbar
    QToolBar *toolBar = new QToolBar("Toolbar", this);
#if defined(Q_OS_MAC)
    toolBar->setIconSize(QSize(20, 20));
#endif
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolBar->addAction(connectButtonAction);
    toolBar->setShortcutEnabled(1, true);
    toolBar->setMovable(false);
    addToolBar(toolBar);

    _execToolBar = new QToolBar("Exec Toolbar", this);
#if defined(Q_OS_MAC)
    _execToolBar->setIconSize(QSize(20, 20));
#endif
    _execToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    _execToolBar->addAction(_executeAction);
    _execToolBar->addAction(_stopAction);
    _execToolBar->addAction(_orientationAction);
    _execToolBar->setShortcutEnabled(1, true);
    _execToolBar->setMovable(false);
    addToolBar(_execToolBar);
    _execToolBar->hide();

//    _miscToolBar = new QToolBar("Misc Toolbar", this);
//    _miscToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
//    _miscToolBar->addAction(customModeAction);
//    _miscToolBar->addAction(treeModeAction);
//    _miscToolBar->addAction(textModeAction);
//    _miscToolBar->setShortcutEnabled(1, true);
//    _miscToolBar->setMovable(false);
//    addToolBar(_miscToolBar);



    // Log Button
    //_status = new QLabel;
    //QPushButton *log = new QPushButton("Logs", this);
    //log->setCheckable(true);
    //connect(log, SIGNAL(toggled(bool)), this, SLOT(toggleLogs(bool)));
    //statusBar()->insertWidget(0, log);
    //statusBar()->addPermanentWidget(_status);

    statusBar();

    createTabs();
    createDatabaseExplorer();

    setWindowTitle("Robomongo " + AppRegistry::instance().version());
    setWindowIcon(GuiRegistry::instance().mainWindowIcon());

    QTimer::singleShot(0, this, SLOT(manageConnections()));
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F12) {
        QRect scr = QApplication::desktop()->screenGeometry();
        _connectionsMenu->move( scr.center() - _connectionsMenu->rect().center() - QPoint(0, 100));
        _connectionsMenu->show();
        return;
    }

    QMainWindow::keyPressEvent(event);
}

void MainWindow::updateConnectionsMenu()
{
    _connectionsMenu->clear();

    int number = 1;
    // Populate list with connections
    foreach(ConnectionSettings *connection, _settingsManager->connections()) {
        QAction *action = new QAction(connection->getReadableName(), this);
        action->setData(QVariant::fromValue(connection));

        if (number <= 9)
            action->setShortcut(QKeySequence(QString("Alt+").append(QString::number(number))));

        _connectionsMenu->addAction(action);
        ++number;
    }

    if (_settingsManager->connections().size() > 0)
        _connectionsMenu->addSeparator();

    // Connect action
    QAction *connectAction = new QAction("&Manage Connections...", this);
    connectAction->setIcon(GuiRegistry::instance().connectIcon());
    connectAction->setToolTip("Connect to MongoDB");
    connect(connectAction, SIGNAL(triggered()), this, SLOT(manageConnections()));

    _connectionsMenu->addAction(connectAction);
}

void MainWindow::manageConnections()
{
    ConnectionsDialog dialog(_settingsManager);
    int result = dialog.exec();

    // save settings and update connection menu
    _settingsManager->save();
    updateConnectionsMenu();

    if (result == QDialog::Accepted) {
        ConnectionSettings *selected = dialog.selectedConnection();

        try {
            _app->openServer(selected->clone(), true);
        } catch(MongoException &ex) {
            QString message = QString("Cannot connect to MongoDB (%1)").arg(selected->getFullAddress());
            QMessageBox::information(this, "Error", message);
        }
    }

    // on linux focus is lost - we need to activate main window back
    activateWindow();
}

void MainWindow::toggleOrientation()
{
    if (_workArea)
        _workArea->toggleOrientation();
}

void MainWindow::enterTextMode()
{
    _viewMode = Text;
    saveViewMode();
    if (_workArea)
        _workArea->enterTextMode();
}

void MainWindow::enterTreeMode()
{
    _viewMode = Tree;
    saveViewMode();
    if (_workArea)
        _workArea->enterTreeMode();
}

void MainWindow::enterCustomMode()
{
    _viewMode = Custom;
    saveViewMode();
    if (_workArea)
        _workArea->enterCustomMode();
}

void MainWindow::saveViewMode()
{
    _settingsManager->setViewMode(_viewMode);
    _settingsManager->save();
}

void MainWindow::executeScript()
{
    QAction *action = static_cast<QAction *>(sender());

    if (action->data().toString() == "Execute") {
        if (_workArea)
            _workArea->executeScript();
    } else {
        stopScript();
    }
}

void MainWindow::stopScript()
{
    if (_workArea)
        _workArea->stopScript();
}

void MainWindow::toggleFullScreen2()
{
    if (windowState() == Qt::WindowFullScreen)
        showNormal();
    else
        showFullScreen();
}

void MainWindow::refreshConnections()
{
    QToolTip::showText(QPoint(0,0),
                       QString("Refresh not working yet... : <br/>  <b>Ctrl+D</b> : push Button"));
}

void MainWindow::aboutRobomongo()
{
    AboutDialog dlg(this);
    dlg.exec();
}

void MainWindow::setDefaultUuidEncoding()
{
    _settingsManager->setUuidEncoding(DefaultEncoding);
    _settingsManager->save();
}

void MainWindow::setJavaUuidEncoding()
{
    _settingsManager->setUuidEncoding(JavaLegacy);
    _settingsManager->save();
}

void MainWindow::setCSharpUuidEncoding()
{
    _settingsManager->setUuidEncoding(CSharpLegacy);
    _settingsManager->save();
}

void MainWindow::setPythonUuidEncoding()
{
    _settingsManager->setUuidEncoding(PythonLegacy);
    _settingsManager->save();
}

void MainWindow::toggleLogs(bool show)
{
    _logDock->setVisible(show);
}

void MainWindow::connectToServer(QAction *connectionAction)
{
    QVariant data = connectionAction->data();
    ConnectionSettings *ptr = data.value<ConnectionSettings *>();

    try
    {
        _app->openServer(ptr->clone(), true);
    }
    catch(MongoException &ex)
    {
        QString message = QString("Cannot connect to MongoDB (%1)").arg(ptr->getFullAddress());
        QMessageBox::information(this, "Error", message);
    }
}

void MainWindow::handle(ConnectionFailedEvent *event)
{
    ConnectionSettings *connection = event->server->connectionRecord();
    QString message = QString("Cannot connect to MongoDB (%1)").arg(connection->getFullAddress());
    QMessageBox::information(this, "Error", message);
}

void MainWindow::handle(ScriptExecutingEvent *event)
{
    _stopAction->setDisabled(false);
    _executeAction->setDisabled(true);
//    _executeAction->setData("Stop");
//    _executeAction->setIcon(GuiRegistry::instance().stopIcon());
//    _executeAction->setIconText("");
//    _executeAction->setShortcut(Qt::Key_F6);
//    _executeAction->setToolTip("Stop currently executed script. <b>(F6)</b>");
}

void MainWindow::handle(ScriptExecutedEvent *event)
{
    _stopAction->setDisabled(true);
    _executeAction->setDisabled(false);
//    _executeAction->setData("Execute");
//    _executeAction->setIcon(GuiRegistry::instance().executeIcon());
//    _executeAction->setIconText("");
//    _executeAction->setShortcut(Qt::Key_F5);
//    _executeAction->setToolTip("Execute query for current tab. If you have some selection in query text - only selection will be executed <b>(F5 </b> or <b>Ctrl + Enter)</b>");
}

void MainWindow::handle(AllTabsClosedEvent *event)
{
    _execToolBar->hide();
}

void MainWindow::handle(QueryWidgetUpdatedEvent *event)
{
    _execToolBar->show();
    _orientationAction->setVisible(event->numOfResults() > 1);
}

void MainWindow::createDatabaseExplorer()
{
    _explorer = new ExplorerWidget(this);
    QDockWidget *explorerDock = new QDockWidget(tr(" Database Explorer"));
    explorerDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    explorerDock->setWidget(_explorer);

    QWidget *titleWidget = new QWidget(this);         // this lines simply removes
    explorerDock->setTitleBarWidget(titleWidget);     // title bar widget.

    addDockWidget(Qt::LeftDockWidgetArea, explorerDock);

    _log = new LogWidget(this);
    _logDock = new QDockWidget(tr(" Log"));
    _logDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    _logDock->setWidget(_log);
    _logDock->setVisible(false);
    addDockWidget(Qt::BottomDockWidgetArea, _logDock);
}

void MainWindow::createTabs()
{
    //TestStackPanel *panel = new TestStackPanel();

    _workArea = new WorkAreaWidget(this);
    setCentralWidget(_workArea);

    //QLabel *label = new QLabel("muahahah");
    //setCentralWidget(label);
}


void ConnectionMenu::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F12) {
        hide();
        return;
    }

    QMenu::keyPressEvent(event);
}
