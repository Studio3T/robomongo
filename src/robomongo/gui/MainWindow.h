#pragma once

#include <QMainWindow>
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
    class OperationFailedEvent;

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
        void toggleAutoExec();
        void toggleLineNumbers();
        void executeScript();
        void stopScript();
        void toggleFullScreen2();
        void selectNextTab();
        void selectPrevTab();
        void duplicateTab();
        void refreshConnections();
        void aboutRobomongo();
        void open();
        void save();
        void saveAs();
        void changeStyle(QAction *);

        void setDefaultUuidEncoding();
        void setJavaUuidEncoding();
        void setCSharpUuidEncoding();
        void setPythonUuidEncoding();
        void setShellAutocompletionAll();
        void setShellAutocompletionNoCollectionNames();
        void setShellAutocompletionNone();
        void setLoadMongoRcJs();
        void setDisableConnectionShortcuts();

        void toggleLogs(bool show);
        void connectToServer(QAction *action);
        void handle(ConnectionFailedEvent *event);
        void handle(ScriptExecutingEvent *event);
        void handle(ScriptExecutedEvent *event);
        void handle(QueryWidgetUpdatedEvent *event);
        void handle(OperationFailedEvent *event);

    protected:
        void closeEvent(QCloseEvent *event);

    private Q_SLOTS:
        void updateMenus();
        void setUtcTimeZone();
        void setLocalTimeZone();
        void openPreferences();
        void openExportDialog();

        void onConnectToolbarVisibilityChanged(bool isVisisble);
        void onOpenSaveToolbarVisibilityChanged(bool isVisisble);
        void onExecToolbarVisibilityChanged(bool isVisisble);
        void onExplorerVisibilityChanged(bool isVisisble);

    private:
        void updateConnectionsMenu();
        void createDatabaseExplorer();
        void createTabs();
        void createStylesMenu();
        void createStatusBar();
        void restoreWindowsSettings();
        void saveWindowsSettings() const;

        QDockWidget *_logDock;

        WorkAreaTabWidget *_workArea;

        App *_app;

        ConnectionMenu *_connectionsMenu;
        QToolButton *_connectButton;
        QMenu *_viewMenu;
        QMenu *_toolbarsMenu;
        QAction *_connectAction;
        // Open/Save tool bar
        QAction *_openAction;
        QAction *_saveAction;
        QAction *_saveAsAction;
        // Execution tool bar
        QAction *_executeAction;
        QAction *_stopAction;
        QAction *_orientationAction;
        QToolBar *_execToolBar;
        // Export/import tool bar
        QAction *_exportAction;
        QAction *_importAction;
    };

}
