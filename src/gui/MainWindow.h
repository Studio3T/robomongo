#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <QWidget>
#include "Core.h"


namespace Robomongo
{
    class LogWidget;
    class ExplorerWidget;
    class MongoManager;
    class SettingsManager;
    class Dispatcher;
    class ConnectionFailedEvent;
    class WorkAreaWidget;

    class MainWindow : public QMainWindow
    {
        Q_OBJECT
    public:
        explicit MainWindow();
        bool event(QEvent *event);

    private:
        /*
        ** Status
        */
        QLabel *_status;

        /*
        ** Log panel
        */
        LogWidget *_log;
        QDockWidget *_logDock;

        WorkAreaWidget *_workArea;

        /*
        ** The only Explorer in the window
        */
        ExplorerWidget *_explorer;

        /**
         * @brief MongoManager
         */
        App &_app;
        SettingsManager &_settingsManager;
        Dispatcher &_dispatcher;

        void createDatabaseExplorer();
        void createTabs();

    signals:

    public slots:
        void manageConnections();
        void refreshConnections();
        void toggleLogs(bool show);

    private:
        void handle(ConnectionFailedEvent *event);
    };

}

#endif // MAINWINDOW_H
