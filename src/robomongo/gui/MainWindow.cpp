#include "robomongo/gui/MainWindow.h"

#include <QApplication>
#include <QToolButton>
#include <QMessageBox>
#include <QWidgetAction>
#include <QMenuBar>
#include <QMenu>
#include <QKeyEvent>
#include <QToolBar>
#include <QToolTip>
#include <QDockWidget>
#include <QDesktopWidget>

#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/utils/QtUtils.h"

#include "robomongo/gui/widgets/LogWidget.h"
#include "robomongo/gui/widgets/explorer/ExplorerWidget.h"
#include "robomongo/gui/widgets/workarea/WorkAreaWidget.h"
#include "robomongo/gui/widgets/workarea/QueryWidget.h"
#include "robomongo/gui/dialogs/ConnectionsDialog.h"
#include "robomongo/gui/dialogs/AboutDialog.h"
#include "robomongo/gui/GuiRegistry.h"

namespace
{
    void setToolBarIconSize(QToolBar *toolBar)
    {
#if defined(Q_OS_MAC)
        const int size = 20;
#else
        const int size = 24;
#endif
        toolBar->setIconSize(QSize(size, size));
    }
}

namespace Robomongo
{
    class ConnectionMenu : public QMenu
    {
    public:
        ConnectionMenu(QWidget *parent) : QMenu(parent) {}
    protected:
        virtual void keyPressEvent(QKeyEvent *event)
        {
            if (event->key() == Qt::Key_F12) {
                hide();
            }
            else {
                QMenu::keyPressEvent(event);
            }
        }
    };

    MainWindow::MainWindow()
        : baseClass(),
        _app(AppRegistry::instance().app()),
        _bus(AppRegistry::instance().bus()),
        _workArea(NULL),
        _connectionsMenu(NULL),
        _viewMode(Custom)
    {
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

        _openAction = new QAction(GuiRegistry::instance().openIcon(), tr("&Open..."), this);
        _openAction->setToolTip("Load script from the file to the currently opened shell");
        VERIFY(connect(_openAction, SIGNAL(triggered()), this, SLOT(open())));

        _saveAction = new QAction(GuiRegistry::instance().saveIcon(), tr("&Save"), this);
        _saveAction->setShortcuts(QKeySequence::Save);
        _saveAction->setToolTip("Save script of the currently opened shell to the file <b>(Ctrl + S)</b>");
        VERIFY(connect(_saveAction, SIGNAL(triggered()), this, SLOT(save())));

        _saveAsAction = new QAction(tr("Save &As..."), this);
        _saveAsAction->setShortcuts(QKeySequence::SaveAs);
        VERIFY(connect(_saveAsAction, SIGNAL(triggered()), this, SLOT(saveAs())));

        // Exit action
        QAction *exitAction = new QAction("&Exit", this);
        exitAction->setShortcut(QKeySequence::Quit);
        VERIFY(connect(exitAction, SIGNAL(triggered()), this, SLOT(close())));

        // Connect action
        _connectAction = new QAction("&Connect", this);
        _connectAction->setShortcut(QKeySequence::Open);
        _connectAction->setIcon(GuiRegistry::instance().connectIcon());
        _connectAction->setIconText("Connect");
        _connectAction->setToolTip("Connect to local or remote MongoDB instance <b>(Ctrl + O)</b>");
        VERIFY(connect(_connectAction, SIGNAL(triggered()), this, SLOT(manageConnections())));

        _connectionsMenu = new ConnectionMenu(this);
        VERIFY(connect(_connectionsMenu, SIGNAL(triggered(QAction*)), this, SLOT(connectToServer(QAction*))));
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

        VERIFY(connect(connectButton, SIGNAL(clicked()), this, SLOT(manageConnections())));

        QWidgetAction *connectButtonAction = new QWidgetAction(this);
        connectButtonAction->setDefaultWidget(connectButton);

        // Orientation action
        _orientationAction = new QAction("&Rotate", this);
        _orientationAction->setShortcut(Qt::Key_F10);
        _orientationAction->setIcon(GuiRegistry::instance().rotateIcon());
        _orientationAction->setToolTip("Toggle orientation of results view <b>(F10)</b>");
        VERIFY(connect(_orientationAction, SIGNAL(triggered()), this, SLOT(toggleOrientation())));

        // read view mode setting
        _viewMode = AppRegistry::instance().settingsManager()->viewMode();

        // Text mode action
        QAction *textModeAction = new QAction("&Text Mode", this);
        textModeAction->setShortcut(Qt::Key_F4);
        textModeAction->setIcon(GuiRegistry::instance().textHighlightedIcon());
        textModeAction->setToolTip("Show current tab in text mode, and make this mode default for all subsequent queries <b>(F4)</b>");
        textModeAction->setCheckable(true);
        textModeAction->setChecked(_viewMode == Text);
        VERIFY(connect(textModeAction, SIGNAL(triggered()), this, SLOT(enterTextMode())));

        // Tree mode action
        QAction *treeModeAction = new QAction("&Tree Mode", this);
        treeModeAction->setShortcut(Qt::Key_F3);
        treeModeAction->setIcon(GuiRegistry::instance().treeHighlightedIcon());
        treeModeAction->setToolTip("Show current tab in tree mode, and make this mode default for all subsequent queries <b>(F3)</b>");
        treeModeAction->setCheckable(true);
        treeModeAction->setChecked(_viewMode == Tree);
        VERIFY(connect(treeModeAction, SIGNAL(triggered()), this, SLOT(enterTreeMode())));

        // Custom mode action
        QAction *customModeAction = new QAction("&Custom Mode", this);
        customModeAction->setShortcut(Qt::Key_F2);
        customModeAction->setIcon(GuiRegistry::instance().customHighlightedIcon());
        customModeAction->setToolTip("Show current tab in custom mode if possible, and make this mode default for all subsequent queries <b>(F2)</b>");
        customModeAction->setCheckable(true);
        customModeAction->setChecked(_viewMode == Custom);
        VERIFY(connect(customModeAction, SIGNAL(triggered()), this, SLOT(enterCustomMode())));

        // Execute action
        _executeAction = new QAction("", this);
        _executeAction->setData("Execute");
        _executeAction->setIcon(GuiRegistry::instance().executeIcon());
        _executeAction->setShortcut(Qt::Key_F5);
        _executeAction->setToolTip("Execute query for current tab. If you have some selection in query text - only selection will be executed <b>(F5 </b> or <b>Ctrl + Enter)</b>");
        VERIFY(connect(_executeAction, SIGNAL(triggered()), SLOT(executeScript())));

        // Stop action
        _stopAction = new QAction("", this);
        _stopAction->setData("Stop");
        _stopAction->setIcon(GuiRegistry::instance().stopIcon());
        _stopAction->setShortcut(Qt::Key_F6);
        _stopAction->setToolTip("Stop execution of currently running script. <b>(F6)</b>");
        _stopAction->setDisabled(true);
        VERIFY(connect(_stopAction, SIGNAL(triggered()), SLOT(stopScript())));

        // Full screen action
        QAction *fullScreenAction = new QAction("&Full Screen", this);
        fullScreenAction->setShortcut(Qt::Key_F11);
        fullScreenAction->setVisible(true);
        VERIFY(connect(fullScreenAction, SIGNAL(triggered()), this, SLOT(toggleFullScreen2())));
        fullScreenAction->setVisible(false);

        // Refresh action
        QAction *refreshAction = new QAction("Refresh", this);
        refreshAction->setIcon(qApp->style()->standardIcon(QStyle::SP_BrowserReload));
        VERIFY(connect(refreshAction, SIGNAL(triggered()), this, SLOT(refreshConnections())));

        // File menu
        QMenu *fileMenu = menuBar()->addMenu("File");
        fileMenu->addAction(_connectAction);
        fileMenu->addAction(fullScreenAction);
        fileMenu->addSeparator();
        fileMenu->addAction(_openAction);
        fileMenu->addAction(_saveAction);
        fileMenu->addAction(_saveAsAction);
        fileMenu->addSeparator();
        fileMenu->addAction(exitAction);

        // Options menu
        QMenu *optionsMenu = menuBar()->addMenu("Options");

        // View Mode
        QMenu *defaultViewModeMenu = optionsMenu->addMenu("Default View Mode");
        defaultViewModeMenu->addAction(customModeAction);
        defaultViewModeMenu->addAction(treeModeAction);
        defaultViewModeMenu->addAction(textModeAction);
        optionsMenu->addSeparator();

        QActionGroup *modeGroup = new QActionGroup(this);
        modeGroup->addAction(textModeAction);
        modeGroup->addAction(treeModeAction);
        modeGroup->addAction(customModeAction);

        // Time Zone
        QAction *utcTime = new QAction("UTC", this);
        utcTime->setCheckable(true);
        utcTime->setChecked(AppRegistry::instance().settingsManager()->timeZone() == Utc);
        VERIFY(connect(utcTime, SIGNAL(triggered()), this, SLOT(setUtcTimeZone())));

        QAction *localTime = new QAction("Local Timezone", this);
        localTime->setCheckable(true);
        localTime->setChecked(AppRegistry::instance().settingsManager()->timeZone() == LocalTime);
        VERIFY(connect(localTime, SIGNAL(triggered()), this, SLOT(setLocalTimeZone())));

        QMenu *timeMenu = optionsMenu->addMenu("Display Dates in ");
        timeMenu->addAction(utcTime);
        timeMenu->addAction(localTime);

        QActionGroup *timeZoneGroup = new QActionGroup(this);
        timeZoneGroup->addAction(utcTime);
        timeZoneGroup->addAction(localTime);

        // UUID encoding
        QAction *defaultEncodingAction = new QAction("Do not decode (show as is)", this);
        defaultEncodingAction->setCheckable(true);
        defaultEncodingAction->setChecked(AppRegistry::instance().settingsManager()->uuidEncoding() == DefaultEncoding);
        VERIFY(connect(defaultEncodingAction, SIGNAL(triggered()), this, SLOT(setDefaultUuidEncoding())));

        QAction *javaLegacyEncodingAction = new QAction("Use Java Encoding", this);
        javaLegacyEncodingAction->setCheckable(true);
        javaLegacyEncodingAction->setChecked(AppRegistry::instance().settingsManager()->uuidEncoding() == JavaLegacy);
        VERIFY(connect(javaLegacyEncodingAction, SIGNAL(triggered()), this, SLOT(setJavaUuidEncoding())));

        QAction *csharpLegacyEncodingAction = new QAction("Use .NET Encoding", this);
        csharpLegacyEncodingAction->setCheckable(true);
        csharpLegacyEncodingAction->setChecked(AppRegistry::instance().settingsManager()->uuidEncoding() == CSharpLegacy);
        VERIFY(connect(csharpLegacyEncodingAction, SIGNAL(triggered()), this, SLOT(setCSharpUuidEncoding())));

        QAction *pythonEncodingAction = new QAction("Use Python Encoding", this);
        pythonEncodingAction->setCheckable(true);
        pythonEncodingAction->setChecked(AppRegistry::instance().settingsManager()->uuidEncoding() == PythonLegacy);
        VERIFY(connect(pythonEncodingAction, SIGNAL(triggered()), this, SLOT(setPythonUuidEncoding())));

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
        VERIFY(connect(aboutRobomongoAction, SIGNAL(triggered()), this, SLOT(aboutRobomongo())));

        // Options menu
        QMenu *helpMenu = menuBar()->addMenu("Help");
        helpMenu->addAction(aboutRobomongoAction);

        // Toolbar
        QToolBar *connectToolBar = new QToolBar("Toolbar", this);
        connectToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        connectToolBar->addAction(connectButtonAction);
        connectToolBar->setShortcutEnabled(1, true);
        connectToolBar->setMovable(false);
        setToolBarIconSize(connectToolBar);
        addToolBar(connectToolBar);

        QToolBar *openSaveToolBar = new QToolBar("Open/Save ToolBar", this);
        openSaveToolBar->addAction(_openAction);
        openSaveToolBar->addAction(_saveAction);
        openSaveToolBar->setMovable(false);
        setToolBarIconSize(openSaveToolBar);
        addToolBar(openSaveToolBar);

        _execToolBar = new QToolBar("Exec Toolbar", this);
        _execToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        _execToolBar->addAction(_executeAction);
        _execToolBar->addAction(_stopAction);
        _execToolBar->addAction(_orientationAction);
        _execToolBar->setShortcutEnabled(1, true);
        _execToolBar->setMovable(false);
        setToolBarIconSize(_execToolBar);
        addToolBar(_execToolBar);

        _execToolBar->hide();

        statusBar();

        createTabs();
        createDatabaseExplorer();

        setWindowTitle(PROJECT_NAME_TITLE" "PROJECT_VERSION);
        setWindowIcon(GuiRegistry::instance().mainWindowIcon());

        QTimer::singleShot(0, this, SLOT(manageConnections()));
        updateMenus();
    }

    void MainWindow::open()
    {
        if(_workArea)
        {
            QueryWidget * wid = _workArea->currentWidget();
            if(wid){
                wid->openFile();
            }
            else
            {
                QList<ConnectionSettings *>  connections = AppRegistry::instance().settingsManager()->connections();
                if(connections.count()==1){
                    ScriptInfo inf = ScriptInfo(QString());
                    if(inf.loadFromFile()){
                    _app->openShell(connections.at(0)->clone(),inf);
                    }
                }
            }
        }
    }

    void MainWindow::save()
    {
        if(_workArea){
            QueryWidget * wid = _workArea->currentWidget();
            if(wid){
                wid->saveToFile();
            }
        }
    }

    void MainWindow::saveAs()
    {
        if(_workArea){
            QueryWidget * wid = _workArea->currentWidget();
            if(wid){
                wid->savebToFileAs();
            }
        }
    }

    void MainWindow::keyPressEvent(QKeyEvent *event)
    {
        if (event->key() == Qt::Key_F12) {
            QRect scr = QApplication::desktop()->screenGeometry();
            _connectionsMenu->move( scr.center() - _connectionsMenu->rect().center() - QPoint(0, 100));
            _connectionsMenu->show();
            return;
        }

        baseClass::keyPressEvent(event);
    }

    void MainWindow::updateConnectionsMenu()
    {
        _connectionsMenu->clear();
        int number = 1;
        // Populate list with connections
        foreach(ConnectionSettings *connection, AppRegistry::instance().settingsManager()->connections()) {
            QAction *action = new QAction(QtUtils::toQString(connection->getReadableName()), this);
            action->setData(QVariant::fromValue(connection));

            if (number <= 9)
                action->setShortcut(QKeySequence(QString("Alt+").append(QString::number(number))));

            _connectionsMenu->addAction(action);
            ++number;
        }

        if (AppRegistry::instance().settingsManager()->connections().size() > 0)
            _connectionsMenu->addSeparator();

        // Connect action
        QAction *connectAction = new QAction("&Manage Connections...", this);
        connectAction->setIcon(GuiRegistry::instance().connectIcon());
        connectAction->setToolTip("Connect to MongoDB");
        VERIFY(connect(connectAction, SIGNAL(triggered()), this, SLOT(manageConnections())));

        _connectionsMenu->addAction(connectAction);
    }

    void MainWindow::manageConnections()
    {
        ConnectionsDialog dialog(AppRegistry::instance().settingsManager());
        int result = dialog.exec();

        // save settings and update connection menu
        AppRegistry::instance().settingsManager()->save();
        updateConnectionsMenu();

        if (result == QDialog::Accepted) {
            ConnectionSettings *selected = dialog.selectedConnection();

            try {
                _app->openServer(selected->clone(), true);
            } catch(const std::exception &) {
                QString message = QString("Cannot connect to MongoDB (%1)").arg(QtUtils::toQString(selected->getFullAddress()));
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
        AppRegistry::instance().settingsManager()->setViewMode(_viewMode);
        AppRegistry::instance().settingsManager()->save();
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
        AppRegistry::instance().settingsManager()->setUuidEncoding(DefaultEncoding);
        AppRegistry::instance().settingsManager()->save();
    }

    void MainWindow::setJavaUuidEncoding()
    {
        AppRegistry::instance().settingsManager()->setUuidEncoding(JavaLegacy);
        AppRegistry::instance().settingsManager()->save();
    }

    void MainWindow::setCSharpUuidEncoding()
    {
        AppRegistry::instance().settingsManager()->setUuidEncoding(CSharpLegacy);
        AppRegistry::instance().settingsManager()->save();
    }

    void MainWindow::setPythonUuidEncoding()
    {
        AppRegistry::instance().settingsManager()->setUuidEncoding(PythonLegacy);
        AppRegistry::instance().settingsManager()->save();
    }

    void MainWindow::setUtcTimeZone()
    {
        AppRegistry::instance().settingsManager()->setTimeZone(Utc);
        AppRegistry::instance().settingsManager()->save();
    }

    void MainWindow::setLocalTimeZone()
    {
        AppRegistry::instance().settingsManager()->setTimeZone(LocalTime);
        AppRegistry::instance().settingsManager()->save();
    }

    void MainWindow::toggleLogs(bool show)
    {
        _logDock->setVisible(show);
    }

    void MainWindow::connectToServer(QAction *connectionAction)
    {
        QVariant data = connectionAction->data();
        ConnectionSettings *ptr = data.value<ConnectionSettings *>();
        try{
            _app->openServer(ptr->clone(), true);
        }
        catch(const std::exception &){
            QString message = QString("Cannot connect to MongoDB (%1)").arg(QtUtils::toQString(ptr->getFullAddress()));
            QMessageBox::information(this, "Error", message);
        }
    }

    void MainWindow::handle(ConnectionFailedEvent *event)
    {
        ConnectionSettings *connection = event->server->connectionRecord();
        QString message = QString("Cannot connect to MongoDB (%1)").arg(QtUtils::toQString(connection->getFullAddress()));
        QMessageBox::information(this, "Error", message);
    }

    void MainWindow::handle(ScriptExecutingEvent *)
    {
        _stopAction->setDisabled(false);
        _executeAction->setDisabled(true);
    }

    void MainWindow::handle(ScriptExecutedEvent *)
    {
        _stopAction->setDisabled(true);
        _executeAction->setDisabled(false);
    }

    void MainWindow::handle(AllTabsClosedEvent *)
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
    void MainWindow::updateMenus()
    {
        bool isEnable = _workArea&&_workArea->countTab()>0;
        _openAction->setEnabled(isEnable);
        _saveAction->setEnabled(isEnable);
        _saveAsAction->setEnabled(isEnable);
    }
    void MainWindow::createTabs()
    {
        _workArea = new WorkAreaWidget(this);
        VERIFY(connect(_workArea, SIGNAL(tabActivated(int)),this, SLOT(updateMenus())));
        setCentralWidget(_workArea);
    }
}
