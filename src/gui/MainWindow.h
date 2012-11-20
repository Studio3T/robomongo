#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <QWidget>

namespace Robomongo
{
    class LogWidget;
    class ExplorerWidget;
    class MongoManager;
    class SettingsManager;


    class MainWindow : public QMainWindow
    {
        Q_OBJECT
    public:
        explicit MainWindow();


    private:
        /*
        ** Status
        */
        QLabel *_status;

        /*
        ** Log panel
        */
        LogWidget *_log;

        /*
        ** The only Explorer in the window
        */
        ExplorerWidget *_explorer;

        /**
         * @brief MongoManager
         */
        MongoManager &_mongoManager;
        SettingsManager &_settingsManager;

        void createDatabaseExplorer();

    signals:

    public slots:
        void manageConnections();
        void refreshConnections();
    };

}

#endif // MAINWINDOW_H
