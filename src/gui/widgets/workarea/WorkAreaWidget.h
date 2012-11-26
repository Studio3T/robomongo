#ifndef WORKAREAWIDGET_H
#define WORKAREAWIDGET_H

#include <QWidget>

namespace Robomongo
{
    class MainWindow;
    class WorkAreaViewModel;
    class WorkAreaTabWidget;
    class QueryWindowViewModel;

    /*
    ** Work Area widget
    */
    class WorkAreaWidget : public QWidget
    {
        Q_OBJECT

    private:

        /*
        ** MainWindow this work area belongs to
        */
        MainWindow * _mainWindow;

        /*
        ** Tab widget
        */
        WorkAreaTabWidget * _tabWidget;

    public:

        /*
        ** Constructs work area
        */
        WorkAreaWidget(MainWindow * mainWindow);

    public slots:

        /*
        ** Handle the moment when query created
        */
        void vm_queryWindowAdded();
    };
}


#endif // WORKAREAWIDGET_H
