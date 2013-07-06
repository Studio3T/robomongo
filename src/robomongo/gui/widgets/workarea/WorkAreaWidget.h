#pragma once
#include <QWidget>

namespace Robomongo
{
    class WorkAreaTabWidget;
    class EventBus;
    class OpeningShellEvent;
    class MainWindow;
    /*
    ** Work Area widget
    */
    class WorkAreaWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit WorkAreaWidget(MainWindow *mainWindow);

        void toggleOrientation();
        void executeScript();
        void stopScript();
        void enterTextMode();
        void enterTreeMode();
        void enterCustomMode();

    public slots:
        void handle(OpeningShellEvent *event);

    private:

        WorkAreaTabWidget *_tabWidget;
        EventBus *_bus;
    };
}
