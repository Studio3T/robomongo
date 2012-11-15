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

class Base {};
class Deli : public Base {};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    //for (int i = 0; i < 1; i ++)
    AppRegistry::instance().settingsManager().save();

    ConnectionsDialog dialog(&AppRegistry::instance().settingsManager());
    dialog.show();

//    ConnectionRecord man1;
//    ConnectionRecord man2;
//    ConnectionRecord man1a = man1;
//    ConnectionRecord man1b = man1a;

//    bool z1 = man1 == man2;
//    bool z2 = man2 == man1a;
//    bool z3 = man1b == man1a;

//    QList<ConnectionRecord> list;
//    list.append(man1);
//    list.append(man2);

//    list.removeOne(man1);
//    list.removeOne(man1a);
//    list.removeOne(man2);

//    QHash<ConnectionRecord, QString> hash;
//    hash.insert(man1, "hello");
//    hash.insert(man2, "aga");

//    hash.remove(man1a);
//    hash.remove(man1);
//    hash.remove(man2);



//    MainWindow w;
//    w.show();

    return app.exec();
}
