#include <QApplication>
//#include <QSettings>
//#include <QList>
#include <QMessageBox>
#include <QList>
#include <QSharedPointer>
#include <QProcess>
//#include <QDir>
//#include "mainwindow.h"
//#include "boost/shared_ptr.hpp"
//#include "boost/scoped_ptr.hpp"
//#include "boost/ptr_container/ptr_vector.hpp"

//#include "qjson/parser.h"
#include "settings/SettingsManager.h"
#include "AppRegistry.h"
#include "settings/ConnectionRecord.h"
#include "dialogs/ConnectionsDialog.h"
#include "Core.h"
#include "MainWindow.h"

using namespace Robomongo;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QProcessEnvironment proc = QProcessEnvironment::systemEnvironment();
    QString str = proc.value("LD_LIBRARY_PATH");

    qDebug() << str;

    //QStringList environment = QProcess::systemEnvironment();



    //for (int i = 0; i < 1; i ++)
    AppRegistry::instance().settingsManager().save();

    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    QSize size(screenGeometry.width() - 450, screenGeometry.height() - 165);

    MainWindow win;
    win.resize(size);

    int x = (screenGeometry.width() - win.width()) / 2;
    int y = (screenGeometry.height() - win.height()) / 2;
    win.move(x, y);
    win.show();

//    ConnectionsDialog dialog(&AppRegistry::instance().settingsManager());
//    dialog.exec();

    //AppRegistry::instance().settingsManager().save();

//    MainWindow w;
//    w.show();

    return app.exec();
}
