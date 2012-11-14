#include <QApplication>
//#include <QSettings>
//#include <QList>
#include <QMessageBox>
#include <QList>
//#include <QDir>
//#include "mainwindow.h"
//#include "boost/shared_ptr.hpp"
//#include "boost/scoped_ptr.hpp"
//#include "boost/ptr_container/ptr_vector.hpp"

//#include "qjson/parser.h"
#include "settings/SettingsManager.h"
#include "AppRegistry.h"
#include "settings/ConnectionRecord.h"
#include "Dialogs/ConnectionsDialog.h"
//#include "Core.h"

using namespace Robomongo;

int main(int argc, char *argv[])
{

    QApplication app(argc, argv);

    //for (int i = 0; i < 1; i ++)
    AppRegistry::instance().settingsManager().save();

/*    QMessageBox box;
    box.setText("Hello");
    box.show();*/

    ConnectionRecord record;
    record.setConnectionName("Hello");

    ConnectionRecord another;
    another.setConnectionName("Another");
    another.setDatabasePort(345);

    QList<ConnectionRecord> records;
    records.append(record);
    records.append(another);

    ConnectionsDialog dialog(records);
    dialog.show();



//    MainWindow w;
//    w.show();

    return app.exec();
}
