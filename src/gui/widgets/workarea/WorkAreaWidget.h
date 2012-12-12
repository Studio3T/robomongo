#ifndef WORKAREAWIDGET_H
#define WORKAREAWIDGET_H

#include <QWidget>

namespace Robomongo
{
    class MainWindow;
    class WorkAreaViewModel;
    class WorkAreaTabWidget;
    class QueryWindowViewModel;
    class Dispatcher;
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

        bool event(QEvent *event);

        void toggleOrientation();

    private: //handlers:
        void handle(const OpeningShellEvent *event);


    private:

        /*
        ** MainWindow this work area belongs to
        */
        MainWindow * _mainWindow;

        /*
        ** Tab widget
        */
        WorkAreaTabWidget * _tabWidget;

        Dispatcher *_dispatcher;

    };
}


#endif // WORKAREAWIDGET_H
