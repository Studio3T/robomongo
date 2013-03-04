#pragma once

#include <QWidget>

namespace Robomongo
{
    class MainWindow;
    class WorkAreaViewModel;
    class WorkAreaTabWidget;
    class QueryWindowViewModel;
    class EventBus;
    class OpeningShellEvent;

    /*
    ** Work Area widget
    */
    class WorkAreaWidget : public QWidget
    {
        Q_OBJECT

    public:
        WorkAreaWidget(MainWindow *mainWindow);
        ~WorkAreaWidget() {}

        void toggleOrientation();
        void executeScript();
        void stopScript();
        void enterTextMode();
        void enterTreeMode();
        void enterCustomMode();

    public slots:
        void handle(OpeningShellEvent *event);

    private:
        /*
        ** MainWindow this work area belongs to
        */
        MainWindow *_mainWindow;

        WorkAreaTabWidget *_tabWidget;
        EventBus *_bus;
    };
}
