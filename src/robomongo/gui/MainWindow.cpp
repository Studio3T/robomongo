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
    
    void saveAutoExpand(bool isExpand)
    {
        Robomongo::AppRegistry::instance().settingsManager()->setAutoExpand(isExpand);
        Robomongo::AppRegistry::instance().settingsManager()->save();
    }

    void saveLineNumbers(bool showLineNumbers)
    {
        Robomongo::AppRegistry::instance().settingsManager()->setLineNumbers(showLineNumbers);
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

        _openAction = new QAction(this);
        _openAction->setIcon(GuiRegistry::instance().openIcon());
        _openAction->setShortcuts(QKeySequence::Open);
        VERIFY(connect(_openAction, SIGNAL(triggered()), this, SLOT(open())));

        _saveAction = new QAction(this);
        _saveAction->setIcon(GuiRegistry::instance().saveIcon());
        _saveAction->setShortcuts(QKeySequence::Save);
        VERIFY(connect(_saveAction, SIGNAL(triggered()), this, SLOT(save())));

        _saveAsAction = new QAction(this);
        _saveAsAction->setShortcuts(QKeySequence::SaveAs);
        VERIFY(connect(_saveAsAction, SIGNAL(triggered()), this, SLOT(saveAs())));

        // Exit action
        _exitAction = new QAction(this);
        _exitAction->setShortcut(QKeySequence::Quit);
        VERIFY(connect(_exitAction, SIGNAL(triggered()), this, SLOT(close())));

        // Connect action
        _connectAction = new QAction(this);
        _connectAction->setShortcut(QKeySequence::New);
        _connectAction->setIcon(GuiRegistry::instance().connectIcon());
        VERIFY(connect(_connectAction, SIGNAL(triggered()), this, SLOT(manageConnections())));

        _connectionsMenu = new ConnectionMenu(this);
        VERIFY(connect(_connectionsMenu, SIGNAL(triggered(QAction*)), this, SLOT(connectToServer(QAction*))));
        updateConnectionsMenu();

        _connectButton = new QToolButton();
        _connectButton->setIcon(GuiRegistry::instance().connectIcon());
        _connectButton->setFocusPolicy(Qt::NoFocus);
        _connectButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        
    #if !defined(Q_OS_MAC)
        _connectButton->setMenu(_connectionsMenu);
        _connectButton->setPopupMode(QToolButton::MenuButtonPopup);
    #endif

        VERIFY(connect(_connectButton, SIGNAL(clicked()), this, SLOT(manageConnections())));

        QWidgetAction *connectButtonAction = new QWidgetAction(this);
        connectButtonAction->setDefaultWidget(_connectButton);

        // Orientation action
        _orientationAction = new QAction(this);
        _orientationAction->setShortcut(Qt::Key_F10);
        _orientationAction->setIcon(GuiRegistry::instance().rotateIcon());
        VERIFY(connect(_orientationAction, SIGNAL(triggered()), this, SLOT(toggleOrientation())));

        // read view mode setting
        ViewMode viewMode = AppRegistry::instance().settingsManager()->viewMode();

        // Text mode action
        _textModeAction = new QAction(this);
        _textModeAction->setShortcut(Qt::Key_F4);
        _textModeAction->setIcon(GuiRegistry::instance().textHighlightedIcon());
        _textModeAction->setCheckable(true);
        _textModeAction->setChecked(viewMode == Text);
        VERIFY(connect(_textModeAction, SIGNAL(triggered()), this, SLOT(enterTextMode())));

        // Tree mode action
        _treeModeAction = new QAction(this);
        _treeModeAction->setShortcut(Qt::Key_F2);
        _treeModeAction->setIcon(GuiRegistry::instance().treeHighlightedIcon());
        _treeModeAction->setCheckable(true);
        _treeModeAction->setChecked(viewMode == Tree);
        VERIFY(connect(_treeModeAction, SIGNAL(triggered()), this, SLOT(enterTreeMode())));

        // Tree mode action
        _tableModeAction = new QAction(this);
        _tableModeAction->setShortcut(Qt::Key_F3);
        _tableModeAction->setIcon(GuiRegistry::instance().tableHighlightedIcon());
        _tableModeAction->setCheckable(true);
        _tableModeAction->setChecked(viewMode == Table);
        VERIFY(connect(_tableModeAction, SIGNAL(triggered()), this, SLOT(enterTableMode())));

        // Custom mode action
        _customModeAction = new QAction(this);
        //_customModeAction->setShortcut(Qt::Key_F2);
        _customModeAction->setIcon(GuiRegistry::instance().customHighlightedIcon());
        _customModeAction->setCheckable(true);
        _customModeAction->setChecked(viewMode == Custom);
        VERIFY(connect(_customModeAction, SIGNAL(triggered()), this, SLOT(enterCustomMode())));

        // Execute action
        _executeAction = new QAction(this);
        _executeAction->setData("Execute");
        _executeAction->setIcon(GuiRegistry::instance().executeIcon());
        _executeAction->setShortcut(Qt::Key_F5);
        VERIFY(connect(_executeAction, SIGNAL(triggered()), SLOT(executeScript())));

        // Stop action
        _stopAction = new QAction(this);
        _stopAction->setData("Stop");
        _stopAction->setIcon(GuiRegistry::instance().stopIcon());
        _stopAction->setShortcut(Qt::Key_F6);
        _stopAction->setDisabled(true);
        VERIFY(connect(_stopAction, SIGNAL(triggered()), SLOT(stopScript())));

        // Refresh action
        _refreshAction = new QAction(this);
        _refreshAction->setIcon(qApp->style()->standardIcon(QStyle::SP_BrowserReload));
        VERIFY(connect(_refreshAction, SIGNAL(triggered()), this, SLOT(refreshConnections())));

    // File menu
        _fileMenu = new QMenu(this);
        _fileMenu->addAction(_connectAction);
        _fileMenu->addSeparator();
        _fileMenu->addAction(_openAction);
        _fileMenu->addAction(_saveAction);
        _fileMenu->addAction(_saveAsAction);
        _fileMenu->addSeparator();
        _fileMenu->addAction(_exitAction);
        menuBar()->addMenu(_fileMenu);

    // View menu
        _viewMenu = new QMenu(this) ;
        menuBar()->addMenu(_viewMenu);
        
    // Options menu
        _optionsMenu = new QMenu(this);
        menuBar()->addMenu(_optionsMenu);

        // View Mode
        _defaultViewModeMenu = new QMenu(this);
        _defaultViewModeMenu->addAction(_customModeAction);
        _defaultViewModeMenu->addAction(_treeModeAction);
        _defaultViewModeMenu->addAction(_tableModeAction);
        _defaultViewModeMenu->addAction(_textModeAction);
        _optionsMenu->addMenu(_defaultViewModeMenu);
        
        _optionsMenu->addSeparator();

        _modeGroup = new QActionGroup(this);
        _modeGroup->addAction(_textModeAction);
        _modeGroup->addAction(_treeModeAction);
        _modeGroup->addAction(_tableModeAction);
        _modeGroup->addAction(_customModeAction);

        // Time Zone
        _utcTimeAction = new QAction(this);
        _utcTimeAction->setCheckable(true);
        _utcTimeAction->setChecked(AppRegistry::instance().settingsManager()->timeZone() == Utc);
        VERIFY(connect(_utcTimeAction, SIGNAL(triggered()), this, SLOT(setUtcTimeZone())));

        _localTimeAction = new QAction(this);
        _localTimeAction->setCheckable(true);
        _localTimeAction->setChecked(AppRegistry::instance().settingsManager()->timeZone() == LocalTime);
        VERIFY(connect(_localTimeAction, SIGNAL(triggered()), this, SLOT(setLocalTimeZone())));

        _timeMenu = new QMenu(this);
        _timeMenu->addAction(_utcTimeAction);
        _timeMenu->addAction(_localTimeAction);
        _optionsMenu->addMenu(_timeMenu);

        _timeZoneGroup = new QActionGroup(this);
        _timeZoneGroup->addAction(_utcTimeAction);
        _timeZoneGroup->addAction(_localTimeAction);

        // UUID encoding
        _defaultEncodingAction = new QAction(this);
        _defaultEncodingAction->setCheckable(true);
        _defaultEncodingAction->setChecked(AppRegistry::instance().settingsManager()->uuidEncoding() == DefaultEncoding);
        VERIFY(connect(_defaultEncodingAction, SIGNAL(triggered()), this, SLOT(setDefaultUuidEncoding())));

        _javaLegacyEncodingAction = new QAction(this);
        _javaLegacyEncodingAction->setCheckable(true);
        _javaLegacyEncodingAction->setChecked(AppRegistry::instance().settingsManager()->uuidEncoding() == JavaLegacy);
        VERIFY(connect(_javaLegacyEncodingAction, SIGNAL(triggered()), this, SLOT(setJavaUuidEncoding())));

        _csharpLegacyEncodingAction = new QAction(this);
        _csharpLegacyEncodingAction->setCheckable(true);
        _csharpLegacyEncodingAction->setChecked(AppRegistry::instance().settingsManager()->uuidEncoding() == CSharpLegacy);
        VERIFY(connect(_csharpLegacyEncodingAction, SIGNAL(triggered()), this, SLOT(setCSharpUuidEncoding())));

        _pythonEncodingAction = new QAction(this);
        _pythonEncodingAction->setCheckable(true);
        _pythonEncodingAction->setChecked(AppRegistry::instance().settingsManager()->uuidEncoding() == PythonLegacy);
        VERIFY(connect(_pythonEncodingAction, SIGNAL(triggered()), this, SLOT(setPythonUuidEncoding())));

        _uuidMenu = new QMenu(this);
        _uuidMenu->addAction(_defaultEncodingAction);
        _uuidMenu->addAction(_javaLegacyEncodingAction);
        _uuidMenu->addAction(_csharpLegacyEncodingAction);
        _uuidMenu->addAction(_pythonEncodingAction);
        _optionsMenu->addMenu(_uuidMenu);

        _loadMongoRcJsAction = new QAction(this);
        _loadMongoRcJsAction->setCheckable(true);
        _loadMongoRcJsAction->setChecked(AppRegistry::instance().settingsManager()->loadMongoRcJs());
        VERIFY(connect(_loadMongoRcJsAction, SIGNAL(triggered()), this, SLOT(setLoadMongoRcJs())));
        _optionsMenu->addSeparator();
        _optionsMenu->addAction(_loadMongoRcJsAction);

        _optionsMenu->addSeparator();

        _autoExpandAction = new QAction(this);
        _autoExpandAction->setCheckable(true);
        _autoExpandAction->setChecked(AppRegistry::instance().settingsManager()->autoExpand());
        VERIFY(connect(_autoExpandAction, SIGNAL(triggered()), this, SLOT(toggleAutoExpand())));
        _optionsMenu->addAction(_autoExpandAction);

        _showLineNumbersAction = new QAction(this);
        _showLineNumbersAction->setCheckable(true);
        _showLineNumbersAction->setChecked(AppRegistry::instance().settingsManager()->lineNumbers());
        VERIFY(connect(_showLineNumbersAction, SIGNAL(triggered()), this, SLOT(toggleLineNumbers())));
        _optionsMenu->addAction(_showLineNumbersAction);
        
        _disabelConnectionShortcutsAction = new QAction(this);
        _disabelConnectionShortcutsAction->setCheckable(true);
        _disabelConnectionShortcutsAction->setChecked(AppRegistry::instance().settingsManager()->disableConnectionShortcuts());
        VERIFY(connect(_disabelConnectionShortcutsAction, SIGNAL(triggered()), this, SLOT(setDisableConnectionShortcuts())));
        _optionsMenu->addAction(_disabelConnectionShortcutsAction);

        _preferencesAction = new QAction(this);
        VERIFY(connect(_preferencesAction, SIGNAL(triggered()), this, SLOT(openPreferences())));
        _preferencesAction->setVisible(false);
        _optionsMenu->addAction(_preferencesAction);
        
        _optionsMenu->addSeparator();
        createLanguagesMenu(_optionsMenu);

        _uuidEncodingGroup = new QActionGroup(this);
        _uuidEncodingGroup->addAction(_defaultEncodingAction);
        _uuidEncodingGroup->addAction(_javaLegacyEncodingAction);
        _uuidEncodingGroup->addAction(_csharpLegacyEncodingAction);
        _uuidEncodingGroup->addAction(_pythonEncodingAction);

    // Window menu
        // Full screen action
        _fullScreenAction = new QAction(this);
    #if !defined(Q_OS_MAC)
        _fullScreenAction->setShortcut(Qt::Key_F11);
    #else
        _fullScreenAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F11));
    #endif
        _fullScreenAction->setVisible(true);
        VERIFY(connect(_fullScreenAction, SIGNAL(triggered()), this, SLOT(toggleFullScreen2())));
        
        // Minimize window
        _minimizeAction = new QAction(this);
        _minimizeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_M));
        _minimizeAction->setVisible(true);
        VERIFY(connect(_minimizeAction, SIGNAL(triggered()), this, SLOT(showMinimized())));
        
        // Next tab
        _nexttabAction = new QAction(this);
        _nexttabAction->setShortcut(QKeySequence(QKeySequence::NextChild));
        _nexttabAction->setVisible(true);
        VERIFY(connect(_nexttabAction, SIGNAL(triggered()), this, SLOT(selectNextTab())));

        // Previous tab
        _prevtabAction = new QAction(this);
        _prevtabAction->setShortcut(QKeySequence(QKeySequence::PreviousChild));
        _prevtabAction->setVisible(true);
        VERIFY(connect(_prevtabAction, SIGNAL(triggered()), this, SLOT(selectPrevTab())));

        // Window menu
        _windowMenu = new QMenu(this);
        
        _windowMenu->addAction(_fullScreenAction);
        _windowMenu->addAction(_minimizeAction);
        _windowMenu->addSeparator();
        _windowMenu->addAction(_nexttabAction);
        _windowMenu->addAction(_prevtabAction);
        menuBar()->addMenu(_windowMenu);
    
    // About menu    
        _aboutRobomongoAction = new QAction(this);
        VERIFY(connect(_aboutRobomongoAction, SIGNAL(triggered()), this, SLOT(aboutRobomongo())));

        // Options menu
        _helpMenu = new QMenu(this);
        _helpMenu->addAction(_aboutRobomongoAction);
        menuBar()->addMenu(_helpMenu);

        // Toolbar
        _connectToolBar = new QToolBar(this);
        _connectToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        _connectToolBar->addAction(connectButtonAction);
        _connectToolBar->setShortcutEnabled(1, true);
        _connectToolBar->setMovable(false);
        setToolBarIconSize(_connectToolBar);
        addToolBar(_connectToolBar);

        _openSaveToolBar = new QToolBar(this);
        _openSaveToolBar->addAction(_openAction);
        _openSaveToolBar->addAction(_saveAction);
        _openSaveToolBar->setMovable(false);
        setToolBarIconSize(_openSaveToolBar);
        addToolBar(_openSaveToolBar);

        _execToolBar = new QToolBar(this);
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
        _viewMenu->addSeparator();
        createStylesMenu();
        createStatusBar();
        setWindowTitle(PROJECT_NAME_TITLE" "PROJECT_VERSION);
        setWindowIcon(GuiRegistry::instance().mainWindowIcon());

        QTimer::singleShot(0, this, SLOT(manageConnections()));
        updateMenus();
        
        retranslateUI();
    }
    
    void MainWindow::retranslateUI()
    {
        QString controlKey = tr("Ctrl");
    #if defined(Q_OS_MAC)
        controlKey = QChar(0x2318); // "Command" key aka Cauliflower
    #endif
        
        _explorerDock->setWindowTitle(tr("Database Explorer"));
        _logDock->setWindowTitle(tr("Logs")); 
        
        _logButton->setText(tr("Logs"));
        
        _openAction->setText(tr("&Open..."));
        _openAction->setToolTip(tr("Load script from the file to the currently opened shell"));
        
        _saveAction->setText(tr("&Save"));
        _saveAction->setToolTip(tr("Save script of the currently opened shell to the file <b>(%1 + S)</b>").arg(controlKey));
        
        _saveAsAction->setText(tr("Save &As..."));

        _exitAction->setText(tr("&Exit"));
        
        _connectAction->setText(tr("&Connect..."));
        _connectAction->setIconText(tr("Connect"));
        _connectAction->setToolTip(tr("Connect to local or remote MongoDB instance <b>(%1 + O)</b>").arg(controlKey));
        
        _connectButton->setText(tr("&Connect..."));
        _connectButton->setToolTip(tr("Connect to local or remote MongoDB instance <b>(%1 + O)</b>").arg(controlKey));
        
        _orientationAction->setText(tr("&Rotate"));
        _orientationAction->setToolTip(tr("Toggle orientation of results view <b>(F10)</b>"));
        
        _textModeAction->setText(tr("&Text Mode"));
        _textModeAction->setToolTip(tr("Show current tab in text mode, and make this mode default for all subsequent queries <b>(F4)</b>"));
        
        _treeModeAction->setText(tr("&Tree Mode"));
        _treeModeAction->setToolTip(tr("Show current tab in tree mode, and make this mode default for all subsequent queries <b>(F3)</b>"));

        _tableModeAction->setText(tr("T&able Mode"));
        _tableModeAction->setToolTip(tr("Show current tab in table mode, and make this mode default for all subsequent queries <b>(F3)</b>"));

        _customModeAction->setText(tr("&Custom Mode"));
        _customModeAction->setToolTip(tr("Show current tab in custom mode if possible, and make this mode default for all subsequent queries <b>(F2)</b>"));

        _executeAction->setToolTip(tr("Execute query for current tab. If you have some selection in query text - only selection will be executed <b>(F5 </b> or <b>%1 + Enter)</b>").arg(controlKey));
        
        _stopAction->setToolTip(tr("Stop execution of currently running script. <b>(F6)</b>"));
        
        _refreshAction->setText(tr("Refresh"));
        
        _utcTimeAction->setText(convertTimesToString(Utc));
        _localTimeAction->setText(convertTimesToString(LocalTime));
        
        _defaultEncodingAction->setText(tr("Do not decode (show as is)"));
        _javaLegacyEncodingAction->setText(tr("Use Java Encoding"));
        _csharpLegacyEncodingAction->setText(tr("Use .NET Encoding"));
        _pythonEncodingAction->setText(tr("Use Python Encoding"));
        
        _loadMongoRcJsAction->setText(tr("Load .mongorc.js"));
        
        _autoExpandAction->setText(tr("Auto Expand First Document"));
        
        _showLineNumbersAction->setText(tr("Show Line Numbers By Default"));
        
        _disabelConnectionShortcutsAction->setText(tr("Disable Connection Shortcuts"));
        
        _preferencesAction->setText(tr("Preferences"));
        
        _aboutRobomongoAction->setText(tr("&About Robomongo..."));
        
        _explorerAction->setText(tr("&Explorer"));
        _explorerAction->setStatusTip(tr("Press to show/hide Database Explorer panel."));
        _logAction->setText(tr("&Logs"));
        //_logAction->setStatusTip(QString("Press to show/hide Logs panel."));  //commented for now because this message hides Logs button in status bar :)
        
        _fileMenu->setTitle(tr("File"));
        _viewMenu->setTitle(tr("View"));
        _stylesMenu->setTitle(tr("Theme"));
        _optionsMenu->setTitle(tr("Options"));
        _defaultViewModeMenu->setTitle(tr("Default View Mode"));
        _timeMenu->setTitle(tr("Display Dates In..."));
        _uuidMenu->setTitle(tr("Legacy UUID Encoding"));
        
        _languagesMenu->setTitle(tr("Language"));
        //: Language based on system locale
        _localeLanguageAction->setText(tr("System locale (if available)"));
       
        _windowMenu->setTitle(tr("Window"));
        _fullScreenAction->setText(tr("&Full Screen"));
        _minimizeAction->setText(tr("&Minimize"));
        _nexttabAction->setText(tr("Select Next Tab"));
        _prevtabAction->setText(tr("Select Previous Tab"));
        
        _helpMenu->setTitle(tr("Help"));
        
        _connectToolBar->setWindowTitle(tr("Toolbar"));
        _openSaveToolBar->setWindowTitle(tr("Open/Save ToolBar"));
        _execToolBar->setWindowTitle(tr("Exec Toolbar"));
        
        updateConnectionsMenu();
    }
    
    void MainWindow::createStylesMenu()
    {
         _stylesMenu = new QMenu(this);
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
             _stylesMenu->addAction(styleAction);             
         }
         _viewMenu->addMenu(_stylesMenu);
    }
    
    void MainWindow::createLanguagesMenu(QMenu *parentMenu)
    {
         _languagesMenu = new QMenu(this);
         QMap<QString, QString> providedTranslations = AppRegistry::instance().settingsManager()->getTranslations();
         const QString &currentTranslation = AppRegistry::instance().settingsManager()->currentTranslation();
         QActionGroup *langGroup = new QActionGroup(this);
         VERIFY(connect(langGroup, SIGNAL(triggered(QAction *)), this, SLOT(changeTranslation(QAction *))));
         for (QMap<QString, QString>::const_iterator it = providedTranslations.begin(); it != providedTranslations.end(); ++it) {
             const QString &language = it.value();
             QAction *langAction = new QAction(language,this);
             if (it.key() == "") {
                 _localeLanguageAction = langAction;
             }
             langAction->setCheckable(true);
             langAction->setChecked(it.key() == currentTranslation);
             langAction->setData(it.key());
             langGroup->addAction(langAction);
             _languagesMenu->addAction(langAction);             
         }
         parentMenu->addMenu(_languagesMenu);
    }

    void MainWindow::createStatusBar()
    {
        QColor windowColor = palette().window().color();
        QColor buttonBgColor = windowColor.lighter(105);
        QColor buttonBorderBgColor = windowColor.darker(120);
        QColor buttonPressedColor = windowColor.darker(102);

        _logButton = new QToolButton(this);
        _logButton->setCheckable(true);
        _logButton->setDefaultAction(_logDock->toggleViewAction());
        _logButton->setStyleSheet(QString(
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

        statusBar()->insertWidget(0, _logButton);
        statusBar()->setStyleSheet("QStatusBar::item { border: 0px solid black };");
    }

    void MainWindow::changeStyle(QAction *ac)
    {
        const QString &text = ac->text();
        detail::applyStyle(text);
        AppRegistry::instance().settingsManager()->setCurrentStyle(text);
        AppRegistry::instance().settingsManager()->save();
    }

    void MainWindow::changeTranslation(QAction *ac)
    {
        const QString &text = ac->text();
        const QString &translation = ac->data().toString();
        AppRegistry::instance().settingsManager()->switchTranslator(translation);
//        QMessageBox::information(this, PROJECT_NAME_TITLE, 
//                tr("You need to restart %1 for language change take effect").arg(PROJECT_NAME_TITLE), 
//                QMessageBox::Ok, 
//                QMessageBox::Ok
//        );
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
                    _app->openShell(connections.at(0), inf);
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
           _connectButton->showMenu();
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
        QAction *connectAction = new QAction(tr("&Manage Connections..."), this);
        connectAction->setIcon(GuiRegistry::instance().connectIcon());
        connectAction->setToolTip(tr("Connect to MongoDB"));
        VERIFY(connect(connectAction, SIGNAL(triggered()), this, SLOT(manageConnections())));

        _connectionsMenu->addAction(connectAction);
    }

    void MainWindow::manageConnections()
    {
        ConnectionsDialog dialog(AppRegistry::instance().settingsManager(), this);
        int result = dialog.exec();

        // save settings and update connection menu
        AppRegistry::instance().settingsManager()->save();
        updateConnectionsMenu();

        if (result == QDialog::Accepted) {
            ConnectionSettings *selected = dialog.selectedConnection();

            try {
                _app->openServer(selected, true);
            } catch(const std::exception &) {
                QString message = tr("Cannot connect to MongoDB (%1)").arg(QtUtils::toQString(selected->getFullAddress()));
                QMessageBox::information(this, tr("Error"), message);
            }
        }

        // on linux focus is lost - we need to activate main window back
        activateWindow();
    }

    void MainWindow::toggleOrientation()
    {
        QueryWidget *widget = _workArea->currentQueryWidget();
        if (!widget)
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

    void MainWindow::toggleAutoExpand()
    {
        QAction *send = qobject_cast<QAction*>(sender());
        saveAutoExpand(send->isChecked());
    }

    void MainWindow::toggleLineNumbers()
    {
        QAction *send = qobject_cast<QAction*>(sender());
        saveLineNumbers(send->isChecked());
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

    void MainWindow::selectNextTab()
    {
        _workArea->nextTab();
    }

    void MainWindow::selectPrevTab()
    {
        _workArea->previousTab();
    }

    void MainWindow::refreshConnections()
    {
        QToolTip::showText(QPoint(0, 0),
                           tr("Refresh not working yet... : <br/>  <b>Ctrl+D</b> : push Button"));
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
            _app->openServer(ptr, true);
        }
        catch(const std::exception &) {
            QString message = tr("Cannot connect to MongoDB (%1)").arg(QtUtils::toQString(ptr->getFullAddress()));
            QMessageBox::information(this, tr("Error"), message);
        }
    }

    void MainWindow::handle(ConnectionFailedEvent *event)
    {
        ConnectionSettings *connection = event->server->connectionRecord();
        QString message = tr("Cannot connect to MongoDB (%1),\nerror: %2").arg(QtUtils::toQString(connection->getFullAddress())).arg(QtUtils::toQString(event->error().errorMessage()));
        QMessageBox::information(this, tr("Error"), message);
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

        _explorerDock = new QDockWidget(this);
        _explorerDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        _explorerDock->setWidget(explorer);
        _explorerDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);

        QWidget *titleWidget = new QWidget(this);         // this lines simply remove
        _explorerDock->setTitleBarWidget(titleWidget);     // title bar widget.

        _explorerAction = _explorerDock->toggleViewAction();
        
        // Adjust any parameter you want.  
        _explorerAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));  
        _explorerAction->setChecked(true);
        // Install action in the menu.  
        _viewMenu->addAction(_explorerAction);

        addDockWidget(Qt::LeftDockWidgetArea, _explorerDock);

        LogWidget *log = new LogWidget(this);        
        VERIFY(connect(&Logger::instance(), SIGNAL(printed(const QString&, mongo::LogLevel)), log, SLOT(addMessage(const QString&, mongo::LogLevel))));
        _logDock = new QDockWidget();
        _logAction = _logDock->toggleViewAction();
        // Adjust any parameter you want.  
        _logAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_L));  
        _logAction->setChecked(false);
        // Install action in the menu.
        _viewMenu->addAction(_logAction);
        
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

    void MainWindow::changeEvent(QEvent* event)
    {
        if (0 != event) {
            switch (event->type()) {
                // this event is send if a translator is loaded
            case QEvent::LanguageChange:
                retranslateUI();
//                Robomongo::LOG_MSG("QEvent::LanguageChange", mongo::LL_INFO);
                break;
                // this event is send, if the system, language changes
            case QEvent::LocaleChange:
                Robomongo::LOG_MSG("QEvent::LocaleChange", mongo::LL_INFO);
                break;
            }
        }
        BaseClass::changeEvent(event);
    }
}
