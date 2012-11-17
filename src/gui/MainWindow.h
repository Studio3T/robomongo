#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <QWidget>

namespace Robomongo
{
    class MainWindow : public QMainWindow
    {
        Q_OBJECT
    public:
        explicit MainWindow();


    private:
        /*
        ** Status
        */
        QLabel * _status;

    signals:

    public slots:
        void manageConnections();
    };

}

#endif // MAINWINDOW_H
