#include <QApplication>
#include <QSettings>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setOrganizationDomain("robomongo.com");
    a.setOrganizationName("Robomongo");
    a.setApplicationName("Robomongo");

    MainWindow w;
    w.show();
    
    return a.exec();
}
