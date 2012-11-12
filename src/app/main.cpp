#include <QApplication>
#include <QSettings>
#include <QList>
#include <QMessageBox>
#include <QDir>
#include "mainwindow.h"
#include "boost/shared_ptr.hpp"
#include "boost/scoped_ptr.hpp"
#include "boost/ptr_container/ptr_vector.hpp"

//#include "qjson/parser.h"
#include "settings/SettingsManager.h"
#include "AppRegistry.h"
//#include "Core.h"

using namespace Robomongo;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    SettingsManager & m = AppRegistry::instance().settingsManager();
    m.save();

    MainWindow w;
    w.show();

    return a.exec();
}
