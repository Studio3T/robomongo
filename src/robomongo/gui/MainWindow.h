#pragma once

#include <QMainWindow>
#include "robomongo/core/events/MongoEventsInfo.hpp"

QT_BEGIN_NAMESPACE
class QLabel;
class QToolBar;
class QDockWidget;
class QToolButton;
QT_END_NAMESPACE

namespace Robomongo
{
    class WorkAreaTabWidget;
    class ConnectionMenu;
    class ExplorerWidget;

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
        void executeScript();
        void stopScript();
        void toggleFullScreen2();
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
        void setLoadMongoRcJs();
        void setDisableConnectionShortcuts();

        void toggleLogs(bool show);
        void connectToServer(QAction *action);
        void startScriptExecute();
        void scriptExecute(const EventsInfo::ExecuteScriptResponceInfo &inf);
        void queryWidgetWindowCountChange(int windowCount);

    private Q_SLOTS:
        void updateMenus();
        void setUtcTimeZone();
        void setLocalTimeZone();
        void openPreferences();
        void connectToServer(const EventsInfo::EstablishConnectionResponceInfo &inf);

    private:
        QDockWidget *_logDock;

        WorkAreaTabWidget *_workArea;
        ExplorerWidget *_explorer;
        ConnectionMenu *_connectionsMenu;
        QToolButton *_connectButton;
        QMenu *_viewMenu;
        QAction *_connectAction;
        QAction *_openAction;
        QAction *_saveAction;
        QAction *_saveAsAction;
        QAction *_executeAction;
        QAction *_stopAction;
        QAction *_orientationAction;
        QToolBar *_execToolBar;

        void updateConnectionsMenu();
        void createDatabaseExplorer();
        void createTabs();
        void createStylesMenu();
        void createStatusBar();
    };

}
