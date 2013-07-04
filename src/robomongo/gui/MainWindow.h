#pragma once

#include <QtGui>
#include <QWidget>
#include <QMainWindow>
#include <QLabel>
#include <QToolBar>
#include <QDockWidget>
#include <QMenu>

#include "robomongo/gui/ViewMode.h"

namespace Robomongo
{
    class LogWidget;
    class ExplorerWidget;
    class MongoManager;
    class SettingsManager;
    class EventBus;
    class ConnectionFailedEvent;
    class ScriptExecutingEvent;
    class ScriptExecutedEvent;
    class QueryWidgetUpdatedEvent;
    class AllTabsClosedEvent;
    class WorkAreaWidget;
    class ConnectionMenu;
	class App;
    class MainWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        MainWindow();
        void keyPressEvent(QKeyEvent *event);
        ViewMode viewMode() const { return _viewMode; }

    public slots:
        void manageConnections();
        void toggleOrientation();
        void enterTextMode();
        void enterTreeMode();
        void enterCustomMode();
        void saveViewMode();
        void executeScript();
        void stopScript();
        void toggleFullScreen2();
        void refreshConnections();
        void aboutRobomongo();

        void setDefaultUuidEncoding();
        void setJavaUuidEncoding();
        void setCSharpUuidEncoding();
        void setPythonUuidEncoding();

        void toggleLogs(bool show);
        void connectToServer(QAction *action);
        void handle(ConnectionFailedEvent *event);
        void handle(ScriptExecutingEvent *event);
        void handle(ScriptExecutedEvent *event);
        void handle(AllTabsClosedEvent *event);
        void handle(QueryWidgetUpdatedEvent *event);

    private:
        QLabel *_status;
        ViewMode _viewMode;
        LogWidget *_log;
        QDockWidget *_logDock;

        WorkAreaWidget *_workArea;

        /*
        ** The only Explorer in the window
        */
        ExplorerWidget *_explorer;

        App *_app;
        SettingsManager *_settingsManager;
        EventBus *_bus;

        ConnectionMenu *_connectionsMenu;
        QAction *_connectAction;
        QAction *_executeAction;
        QAction *_stopAction;
        QAction *_orientationAction;
        QToolBar *_execToolBar;

        void updateConnectionsMenu();
        void createDatabaseExplorer();
        void createTabs();
    };

    class ConnectionMenu : public QMenu
    {
        Q_OBJECT

    public:
        ConnectionMenu(QWidget *parent) : QMenu(parent) {}
        void keyPressEvent(QKeyEvent *event);
    };

}
