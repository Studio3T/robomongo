#pragma once

#include <vector>
#include <QMainWindow>
#include <QActionGroup>
#include "../core/utils/QtUtils.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QToolBar;
class QDockWidget;
class QToolButton;
QT_END_NAMESPACE

        namespace Robomongo
{
    class ConnectionFailedEvent;
    class ScriptExecutingEvent;
    class ScriptExecutedEvent;
    class QueryWidgetUpdatedEvent;
    class WorkAreaTabWidget;
    class ConnectionMenu;
    class App;

    class MainWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        typedef QMainWindow BaseClass;
        MainWindow();
        void keyPressEvent(QKeyEvent *event);

    public Q_SLOTS:
        void manageConnections();
        void toggleOrientation();
        void enterTextMode();
        void enterTreeMode();
        void enterTableMode();
        void enterCustomMode();
        void toggleAutoExpand();
        void toggleLineNumbers();
        void executeScript();
        void stopScript();
        void toggleFullScreen2();
        void selectNextTab();
        void selectPrevTab();
        void refreshConnections();
        void aboutRobomongo();
        void open();
        void save();
        void saveAs();
        void changeStyle(QAction *);
        void changeTranslation(QAction *);

        void setDefaultUuidEncoding();
        void setJavaUuidEncoding();
        void setCSharpUuidEncoding();
        void setPythonUuidEncoding();
        void setLoadMongoRcJs();
        void setDisableConnectionShortcuts();

        void toggleLogs(bool show);
        void connectToServer(QAction *action);
        void handle(ConnectionFailedEvent *event);
        void handle(ScriptExecutingEvent *event);
        void handle(ScriptExecutedEvent *event);
        void handle(QueryWidgetUpdatedEvent *event);

    protected:
        void changeEvent(QEvent* event);

    private Q_SLOTS:
        void updateMenus();
        void setUtcTimeZone();
        void setLocalTimeZone();
        void openPreferences();

    private:
        QDockWidget *_logDock;
        QDockWidget *_explorerDock;

        WorkAreaTabWidget *_workArea;

        App *_app;

        ConnectionMenu *_connectionsMenu;

        QToolButton *_connectButton;
        QToolButton *_logButton;

        QAction *_connectAction;
        QAction *_openAction;
        QAction *_saveAction;
        QAction *_saveAsAction;
        QAction *_exitAction;
        QAction *_orientationAction;
        QAction *_textModeAction;
        QAction *_treeModeAction;
        QAction *_tableModeAction;
        QAction *_customModeAction;
        QAction *_executeAction;
        QAction *_stopAction;
        QAction *_refreshAction;
        QAction *_utcTimeAction;
        QAction *_localTimeAction;
        QAction *_defaultEncodingAction;
        QAction *_javaLegacyEncodingAction;
        QAction *_csharpLegacyEncodingAction;
        QAction *_pythonEncodingAction;
        QAction *_loadMongoRcJsAction;
        QAction *_autoExpandAction;
        QAction *_showLineNumbersAction;
        QAction *_disabelConnectionShortcutsAction;
        QAction *_preferencesAction;
        QAction *_localeLanguageAction;
        
        QAction *_fullScreenAction;
        QAction *_minimizeAction;
        QAction *_nexttabAction;
        QAction *_prevtabAction;
        
        QAction *_aboutRobomongoAction;

        QAction *_explorerAction;
        QAction *_logAction;

        QActionGroup *_modeGroup;
        QActionGroup *_timeZoneGroup;
        QActionGroup *_uuidEncodingGroup;

        QMenu *_fileMenu;
        QMenu *_viewMenu;
        QMenu *_stylesMenu;
        QMenu *_optionsMenu;
        QMenu *_defaultViewModeMenu;
        QMenu *_timeMenu;
        QMenu *_uuidMenu;
        QMenu *_languagesMenu;
        QMenu *_windowMenu;
        QMenu *_helpMenu;

        QToolBar *_execToolBar;
        QToolBar *_connectToolBar;
        QToolBar *_openSaveToolBar;

        void retranslateUI();

        void updateConnectionsMenu();
        void createDatabaseExplorer();
        void createTabs();
        void createStylesMenu();
        void createLanguagesMenu(QMenu *parentMenu);
        void createStatusBar();
    };

}
