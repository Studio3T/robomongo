#pragma once

#include <QMainWindow>
#include <QSystemTrayIcon>

QT_BEGIN_NAMESPACE
class QLabel;
class QToolBar;
class QDockWidget;
class QToolButton;
class QPushButton;
class QTreeWidgetItem;
class QNetworkReply;
class QNetworkAccessManager;
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
    class ExplorerWidget;
    class WelcomeTab;

    class MainWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        typedef QMainWindow BaseClass;
        MainWindow();

        WelcomeTab* getWelcomeTab();

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
        void exit();

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
        void keyPressEvent(QKeyEvent *event) override;
        void closeEvent(QCloseEvent *event) override;
        void hideEvent(QHideEvent *event) override;
        void showEvent(QShowEvent *event) override;
        bool eventFilter(QObject *target, QEvent *event) override;
        void resizeEvent(QResizeEvent* event) override;
        
    private Q_SLOTS:
        void updateMenus();
        void setUtcTimeZone();
        void setLocalTimeZone();
        void openPreferences();
        void openWelcomeTab();

        // Temporarily disabling export/import feature
        //void openExportDialog();

        void onConnectToolbarVisibilityChanged(bool isVisisble);
        void onOpenSaveToolbarVisibilityChanged(bool isVisisble);
        void onExecToolbarVisibilityChanged(bool isVisisble);
        void onExplorerVisibilityChanged(bool isVisisble);

        // Temporarily disabling export/import feature
        //void onExplorerItemSelected(QTreeWidgetItem *selectedItem);

        void on_tabChange();

        void toggleMinimize();
        void trayActivated(QSystemTrayIcon::ActivationReason reason);
        void toggleMinimizeToTray();

        // On application focus changes
        void on_focusChanged();

        void on_networkReply(QNetworkReply* reply);
        void on_closeButton_clicked();
        
        void checkUpdates();
        void toggleCheckUpdates();
        void openShellTimeoutDialog();

    private:
        void updateConnectionsMenu();
        void createDatabaseExplorer();
        void createTabs();
        void createStylesMenu();
        void createStatusBar();
        void restoreWindowSettings();
        void saveWindowSettings() const;
        void adjustUpdatesBarHeight();

        QDockWidget *_logDock;

        WorkAreaTabWidget *_workArea;

        ExplorerWidget* _explorer;

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
        QToolBar *_updateBar;
        QLabel *_updateLabel;
        QPushButton* _closeButton;

        QNetworkAccessManager *_networkAccessManager;

        // Temporarily disabling export/import feature
        /*
        // Export/import tool bar
        QAction *_exportAction;
        QAction *_importAction;
        */

#if defined(Q_OS_WIN)
        QSystemTrayIcon *_trayIcon;
#endif

        bool _allowExit;
        bool _updateMenusAtStart = true;
    };

}
