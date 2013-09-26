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
#include <QTimer>
#include <QPushButton>
#include <QLabel>
#include <QStatusBar>
#include <QHBoxLayout>

#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/utils/Logger.h"

#include "robomongo/gui/widgets/LogWidget.h"
#include "robomongo/gui/widgets/explorer/ExplorerWidget.h"
#include "robomongo/gui/widgets/workarea/WorkAreaTabWidget.h"
#include "robomongo/gui/widgets/workarea/QueryWidget.h"
#include "robomongo/gui/dialogs/ConnectionsDialog.h"
#include "robomongo/gui/dialogs/AboutDialog.h"
#include "robomongo/gui/dialogs/PreferencesDialog.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/AppStyle.h"

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

    void saveViewMode(Robomongo::ViewMode mode)
    {
        Robomongo::AppRegistry::instance().settingsManager()->setViewMode(mode);
        Robomongo::AppRegistry::instance().settingsManager()->save();
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
        : BaseClass(),
        _app(AppRegistry::instance().app()),
        _workArea(NULL),
        _connectionsMenu(NULL)
    {
        AppRegistry::instance().bus()->subscribe(this, ConnectionFailedEvent::Type);
        AppRegistry::instance().bus()->subscribe(this, ScriptExecutedEvent::Type);
        AppRegistry::instance().bus()->subscribe(this, ScriptExecutingEvent::Type);
        AppRegistry::instance().bus()->subscribe(this, QueryWidgetUpdatedEvent::Type);

        QColor background = palette().window().color();

    #if defined(Q_OS_MAC)
        QString explorerColor = "#DEE3EA"; // was #CED6DF"
    #elif defined(Q_OS_LINUX)
        QString explorerColor = background.darker(103).name();
    #else
        QString explorerColor = background.lighter(103).name();
    #endif

        qApp->setStyleSheet(QString(
            "QWidget#queryWidget { background-color:#E7E5E4; margin: 0px; padding:0px; } \n"
            "Robomongo--ExplorerTreeWidget#explorerTree { padding: 1px 0px 0px 0px; background-color: %1; border: 0px; } \n"
            "QMainWindow::separator { background: #E7E5E4; width: 1px; } "
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
        _connectAction = new QAction("&Connect...", this);
        _connectAction->setShortcut(QKeySequence::Open);
        _connectAction->setIcon(GuiRegistry::instance().connectIcon());
        _connectAction->setIconText("Connect");
        _connectAction->setToolTip("Connect to local or remote MongoDB instance <b>(Ctrl + O)</b>");
        VERIFY(connect(_connectAction, SIGNAL(triggered()), this, SLOT(manageConnections())));

        _connectionsMenu = new ConnectionMenu(this);
        VERIFY(connect(_connectionsMenu, SIGNAL(triggered(QAction*)), this, SLOT(connectToServer(QAction*))));
        updateConnectionsMenu();

        QToolButton *connectButton = new QToolButton();
        connectButton->setText("&Connect...");
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
        ViewMode viewMode = AppRegistry::instance().settingsManager()->viewMode();

        // Text mode action
        QAction *textModeAction = new QAction("&Text Mode", this);
        textModeAction->setShortcut(Qt::Key_F4);
        textModeAction->setIcon(GuiRegistry::instance().textHighlightedIcon());
        textModeAction->setToolTip("Show current tab in text mode, and make this mode default for all subsequent queries <b>(F4)</b>");
        textModeAction->setCheckable(true);
        textModeAction->setChecked(viewMode == Text);
        VERIFY(connect(textModeAction, SIGNAL(triggered()), this, SLOT(enterTextMode())));

        // Tree mode action
        QAction *treeModeAction = new QAction("&Tree Mode", this);
        treeModeAction->setShortcut(Qt::Key_F2);
        treeModeAction->setIcon(GuiRegistry::instance().treeHighlightedIcon());
        treeModeAction->setToolTip("Show current tab in tree mode, and make this mode default for all subsequent queries <b>(F3)</b>");
        treeModeAction->setCheckable(true);
        treeModeAction->setChecked(viewMode == Tree);
        VERIFY(connect(treeModeAction, SIGNAL(triggered()), this, SLOT(enterTreeMode())));

        // Tree mode action
        QAction *tableModeAction = new QAction("T&able Mode", this);
        tableModeAction->setShortcut(Qt::Key_F3);
        tableModeAction->setIcon(GuiRegistry::instance().tableHighlightedIcon());
        tableModeAction->setToolTip("Show current tab in table mode, and make this mode default for all subsequent queries <b>(F3)</b>");
        tableModeAction->setCheckable(true);
        tableModeAction->setChecked(viewMode == Table);
        VERIFY(connect(tableModeAction, SIGNAL(triggered()), this, SLOT(enterTableMode())));

        // Custom mode action
        QAction *customModeAction = new QAction("&Custom Mode", this);
        //customModeAction->setShortcut(Qt::Key_F2);
        customModeAction->setIcon(GuiRegistry::instance().customHighlightedIcon());
        customModeAction->setToolTip("Show current tab in custom mode if possible, and make this mode default for all subsequent queries <b>(F2)</b>");
        customModeAction->setCheckable(true);
        customModeAction->setChecked(viewMode == Custom);
        VERIFY(connect(customModeAction, SIGNAL(triggered()), this, SLOT(enterCustomMode())));

        // Execute action
        _executeAction = new QAction(this);
        _executeAction->setData("Execute");
        _executeAction->setIcon(GuiRegistry::instance().executeIcon());
        _executeAction->setShortcut(Qt::Key_F5);
        _executeAction->setToolTip("Execute query for current tab. If you have some selection in query text - only selection will be executed <b>(F5 </b> or <b>Ctrl + Enter)</b>");
        VERIFY(connect(_executeAction, SIGNAL(triggered()), SLOT(executeScript())));

        // Stop action
        _stopAction = new QAction(this);
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

        // Refresh action
        QAction *refreshAction = new QAction("Refresh", this);
        refreshAction->setIcon(qApp->style()->standardIcon(QStyle::SP_BrowserReload));
        VERIFY(connect(refreshAction, SIGNAL(triggered()), this, SLOT(refreshConnections())));

        // File menu
        QMenu *fileMenu = menuBar()->addMenu("File");
        fileMenu->addAction(_connectAction);
        fileMenu->addSeparator();
        fileMenu->addAction(_openAction);
        fileMenu->addAction(_saveAction);
        fileMenu->addAction(_saveAsAction);
        fileMenu->addSeparator();
        fileMenu->addAction(exitAction);

        // View menu
        _viewMenu = menuBar()->addMenu("View");
        
        // Options menu
        QMenu *optionsMenu = menuBar()->addMenu("Options");

        // View Mode
        QMenu *defaultViewModeMenu = optionsMenu->addMenu("Default View Mode");
        defaultViewModeMenu->addAction(customModeAction);
        defaultViewModeMenu->addAction(treeModeAction);
        defaultViewModeMenu->addAction(tableModeAction);
        defaultViewModeMenu->addAction(textModeAction);
        optionsMenu->addSeparator();

        QActionGroup *modeGroup = new QActionGroup(this);
        modeGroup->addAction(textModeAction);
        modeGroup->addAction(treeModeAction);
        modeGroup->addAction(tableModeAction);
        modeGroup->addAction(customModeAction);

        // Time Zone
        QAction *utcTime = new QAction(convertTimesToString(Utc), this);
        utcTime->setCheckable(true);
        utcTime->setChecked(AppRegistry::instance().settingsManager()->timeZone() == Utc);
        VERIFY(connect(utcTime, SIGNAL(triggered()), this, SLOT(setUtcTimeZone())));

        QAction *localTime = new QAction(convertTimesToString(LocalTime), this);
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

        QAction *loadMongoRcJs = new QAction("Load .mongorc.js",this);
        loadMongoRcJs->setCheckable(true);
        loadMongoRcJs->setChecked(AppRegistry::instance().settingsManager()->loadMongoRcJs());
        VERIFY(connect(loadMongoRcJs, SIGNAL(triggered()), this, SLOT(setLoadMongoRcJs())));
        optionsMenu->addSeparator();
        optionsMenu->addAction(loadMongoRcJs);

        QAction *disabelConnectionShortcuts = new QAction("Disable connection shortcuts",this);
        disabelConnectionShortcuts->setCheckable(true);
        disabelConnectionShortcuts->setChecked(AppRegistry::instance().settingsManager()->disableConnectionShortcuts());
        VERIFY(connect(disabelConnectionShortcuts, SIGNAL(triggered()), this, SLOT(setDisableConnectionShortcuts())));
        optionsMenu->addSeparator();
        optionsMenu->addAction(disabelConnectionShortcuts);

        QAction *preferencesAction = new QAction("Preferences",this);
        VERIFY(connect(preferencesAction, SIGNAL(triggered()), this, SLOT(openPreferences())));
        preferencesAction->setVisible(false);
        optionsMenu->addAction(preferencesAction);

        QActionGroup *uuidEncodingGroup = new QActionGroup(this);
        uuidEncodingGroup->addAction(defaultEncodingAction);
        uuidEncodingGroup->addAction(javaLegacyEncodingAction);
        uuidEncodingGroup->addAction(csharpLegacyEncodingAction);
        uuidEncodingGroup->addAction(pythonEncodingAction);

        QAction *aboutRobomongoAction = new QAction("&About Robomongo...", this);
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

        createTabs();
        createDatabaseExplorer();
        createStylesMenu();
        createStatusBar();
        _viewMenu->addAction(fullScreenAction);
        setWindowTitle(PROJECT_NAME_TITLE" "PROJECT_VERSION);
        setWindowIcon(GuiRegistry::instance().mainWindowIcon());

        QTimer::singleShot(0, this, SLOT(manageConnections()));
        updateMenus();
    }

    void MainWindow::createStylesMenu()
    {
        _viewMenu->addSeparator();
         QMenu *styles = _viewMenu->addMenu("Theme");
         QStringList supportedStyles = detail::getSupportedStyles();
         QActionGroup *styleGroup = new QActionGroup(this);
         VERIFY(connect(styleGroup, SIGNAL(triggered(QAction *)), this, SLOT(changeStyle(QAction *))));
         const QString &currentStyle = AppRegistry::instance().settingsManager()->currentStyle();
         for (QStringList::const_iterator it = supportedStyles.begin(); it != supportedStyles.end(); ++it) {
             const QString &style = *it;
             QAction *styleAction = new QAction(style,this);
             styleAction->setCheckable(true);
             styleAction->setChecked(style == currentStyle);
             styleGroup->addAction(styleAction);
             styles->addAction(styleAction);             
         }
    }

    void MainWindow::createStatusBar()
    {
        QColor windowColor = palette().window().color();
        QColor buttonBgColor = windowColor.lighter(105);
        QColor buttonBorderBgColor = windowColor.darker(120);
        QColor buttonPressedColor = windowColor.darker(102);

        QToolButton *log = new QToolButton(this);
        log->setText("Logs");
        log->setCheckable(true);
        log->setDefaultAction(_logDock->toggleViewAction());
        log->setStyleSheet(QString(
            "QToolButton {"
            "   background-color: %1;"
            "   border-style: outset;"
            "   border-width: 1px;"
            "   border-radius: 3px;"
            "   border-color: %2;"
            "   padding: 1px 20px 1px 20px;"
            "} \n"
            ""
            "QToolButton:checked, QToolButton:pressed {"
            "   background-color: %3;"
            "   border-style: inset;"
            "}")
            .arg(buttonBgColor.name())
            .arg(buttonBorderBgColor.name())
            .arg(buttonPressedColor.name()));

        statusBar()->insertWidget(0, log);
        statusBar()->setStyleSheet("QStatusBar::item { border: 0px solid black };");
    }

    void MainWindow::changeStyle(QAction *ac)
    {
        const QString &text = ac->text();
        detail::applyStyle(text);
        AppRegistry::instance().settingsManager()->setCurrentStyle(text);
        AppRegistry::instance().settingsManager()->save();
    }

    void MainWindow::open()
    {
        QueryWidget *wid = _workArea->currentQueryWidget();
        if (wid) {
            wid->openFile();
        }
        else {
            SettingsManager::ConnectionSettingsContainerType connections = AppRegistry::instance().settingsManager()->connections();
            if (connections.size() == 1) {
                ScriptInfo inf = ScriptInfo(QString());
                if (inf.loadFromFile()) {
                    _app->openShell(connections.at(0)->clone(), inf);
                }
            }
        }
    }

    void MainWindow::save()
    {
        QueryWidget *wid = _workArea->currentQueryWidget();
        if (wid) {
            wid->saveToFile();
        }
    }

    void MainWindow::saveAs()
    {
        QueryWidget *wid = _workArea->currentQueryWidget();
        if (wid) {
            wid->savebToFileAs();
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

        BaseClass::keyPressEvent(event);
    }

    void MainWindow::updateConnectionsMenu()
    {
        _connectionsMenu->clear();
        int number = 1;
        // Populate list with connections
        SettingsManager::ConnectionSettingsContainerType connections = AppRegistry::instance().settingsManager()->connections();
        for(SettingsManager::ConnectionSettingsContainerType::const_iterator it = connections.begin(); it!= connections.end(); ++it) {
            ConnectionSettings *connection = *it;
            QAction *action = new QAction(QtUtils::toQString(connection->getReadableName()), this);
            action->setData(QVariant::fromValue(connection));

            if (number <= 9 && !AppRegistry::instance().settingsManager()->disableConnectionShortcuts()) {
                action->setShortcut(QKeySequence(QString("Alt+").append(QString::number(number))));
            }

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
        QueryWidget *widget = _workArea->currentQueryWidget();
        if (widget)
            return;

        widget->toggleOrientation();
    }

    void MainWindow::enterTextMode()
    {
        saveViewMode(Text);
        QueryWidget *widget = _workArea->currentQueryWidget();
        if (!widget)
            return;

        widget->enterTextMode();
    }

    void MainWindow::enterTreeMode()
    {
        saveViewMode(Tree);
        QueryWidget *widget = _workArea->currentQueryWidget();
        if (!widget)
            return;

        widget->enterTreeMode();
    }

    void MainWindow::enterTableMode()
    {
        saveViewMode(Table);
        QueryWidget *widget = _workArea->currentQueryWidget();
        if (!widget)
            return;

        widget->enterTableMode();
    }

    void MainWindow::enterCustomMode()
    {
        saveViewMode(Custom);
        QueryWidget *widget = _workArea->currentQueryWidget();
        if (!widget)
            return;

        widget->enterCustomMode();
    }

    void MainWindow::executeScript()
    {
        QAction *action = static_cast<QAction *>(sender());

        if (action->data().toString() != "Execute") {
            stopScript();
            return;
        }

        QueryWidget *widget = _workArea->currentQueryWidget();
        if (!widget)
            return;

        widget->execute();
    }

    void MainWindow::stopScript()
    {
        QueryWidget *widget = _workArea->currentQueryWidget();
        if (!widget)
            return;

        widget->stop();
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

    void MainWindow::openPreferences()
    {
        PreferencesDialog dlg(this);
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

    void MainWindow::setDisableConnectionShortcuts()
    {
        QAction *send = qobject_cast<QAction*>(sender());
        AppRegistry::instance().settingsManager()->setDisableConnectionShortcuts(send->isChecked());
        AppRegistry::instance().settingsManager()->save();
        updateConnectionsMenu();
    }

    void MainWindow::setLoadMongoRcJs()
    {
        QAction *send = qobject_cast<QAction*>(sender());
        AppRegistry::instance().settingsManager()->setLoadMongoRcJs(send->isChecked());
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
        try {
            _app->openServer(ptr->clone(), true);
        }
        catch(const std::exception &) {
            QString message = QString("Cannot connect to MongoDB (%1)").arg(QtUtils::toQString(ptr->getFullAddress()));
            QMessageBox::information(this, "Error", message);
        }
    }

    void MainWindow::handle(ConnectionFailedEvent *event)
    {
        ConnectionSettings *connection = event->server->connectionRecord();
        QString message = QString("Cannot connect to MongoDB (%1),\nerror: %2").arg(QtUtils::toQString(connection->getFullAddress())).arg(QtUtils::toQString(event->error().errorMessage()));
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

    void MainWindow::handle(QueryWidgetUpdatedEvent *event)
    {
        _orientationAction->setVisible(event->numOfResults() > 1);
    }

    void MainWindow::createDatabaseExplorer()
    {
        ExplorerWidget *explorer = new ExplorerWidget(this);
        AppRegistry::instance().bus()->subscribe(explorer, ConnectingEvent::Type);
        AppRegistry::instance().bus()->subscribe(explorer, ConnectionFailedEvent::Type);
        AppRegistry::instance().bus()->subscribe(explorer, ConnectionEstablishedEvent::Type);

        QDockWidget *explorerDock = new QDockWidget(tr("Database Explorer"));
        explorerDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        explorerDock->setWidget(explorer);
        explorerDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);

        QWidget *titleWidget = new QWidget(this);         // this lines simply remove
        explorerDock->setTitleBarWidget(titleWidget);     // title bar widget.

        QAction *actionExp = explorerDock->toggleViewAction();

        // Adjust any parameter you want.  
        actionExp->setText(QString("&Explorer"));
        actionExp->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));  
        actionExp->setStatusTip(QString("Press to show/hide Database Explorer panel."));
        actionExp->setChecked(true);
        // Install action in the menu.  
        _viewMenu->addAction(actionExp);

        addDockWidget(Qt::LeftDockWidgetArea, explorerDock);

        LogWidget *log = new LogWidget(this);        
        VERIFY(connect(&Logger::instance(), SIGNAL(printed(const QString&, mongo::LogLevel)), log, SLOT(addMessage(const QString&, mongo::LogLevel))));
        _logDock = new QDockWidget(tr("Logs"));
        QAction *action = _logDock->toggleViewAction();
        // Adjust any parameter you want.  
        action->setText(QString("&Logs"));
        action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_L));  
        //action->setStatusTip(QString("Press to show/hide Logs panel."));  //commented for now because this message hides Logs button in status bar :)
        action->setChecked(false);
        // Install action in the menu.
        _viewMenu->addAction(action);

        _logDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
        _logDock->setWidget(log);
        _logDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
        _logDock->setVisible(false);
        addDockWidget(Qt::BottomDockWidgetArea, _logDock);
    }

    void MainWindow::updateMenus()
    {
        int contTab = _workArea->count();
        bool isEnable = _workArea && contTab > 0;

        _execToolBar->setVisible(isEnable);
        _openAction->setEnabled(isEnable);
        _saveAction->setEnabled(isEnable);
        _saveAsAction->setEnabled(isEnable);
    }

    void MainWindow::createTabs()
    {
        _workArea = new WorkAreaTabWidget(this);
        AppRegistry::instance().bus()->subscribe(_workArea, OpeningShellEvent::Type);
        VERIFY(connect(_workArea, SIGNAL(currentChanged(int)), this, SLOT(updateMenus())));

        QHBoxLayout *hlayout = new QHBoxLayout;
        hlayout->setContentsMargins(0, 3, 0, 0);
        hlayout->addWidget(_workArea);
        QWidget *window = new QWidget;
        window->setLayout(hlayout);

        setCentralWidget(window);
    }
}
