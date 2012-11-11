#include <QApplication>
#include <QSettings>
#include <QList>
#include <QMessageBox>
#include <QDir>
#include "mainwindow.h"
#include "boost/shared_ptr.hpp"
#include "boost/ptr_container/ptr_vector.hpp"

#include "qjson/parser.h"
#include "settings/SettingsManager.h"
#include "AppRegistry.h"

using namespace Robomongo;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setOrganizationDomain("robomongo.codm");
    a.setOrganizationName("Robomongo");
    a.setApplicationName("Robomongo");

    QJson::Parser parser;

    bool ok;

    // json is a QString containing the data to convert
    QVariantMap result = parser.parse (" { \"tedsfdddddddddst2\" : \"value\" } ", &ok).toMap();

    SettingsManager * m = AppRegistry::instance().settingsManager();

    m->save();

    MainWindow w;
    w.show();

//    delete manager;
    
    return a.exec();
}
