#ifndef WORKAREAWIDGET_H
#define WORKAREAWIDGET_H

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

        /*
        ** Constructs work area
        */
        WorkAreaWidget(MainWindow * mainWindow);
        ~WorkAreaWidget();

        void toggleOrientation();
        void executeScript();



    public slots:
        void handle(OpeningShellEvent *event);


    private:

        /*
        ** MainWindow this work area belongs to
        */
        MainWindow * _mainWindow;

        /*
        ** Tab widget
        */
        WorkAreaTabWidget * _tabWidget;

        EventBus *_bus;

    };
}


#endif // WORKAREAWIDGET_H
