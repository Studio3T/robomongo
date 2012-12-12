#include <QApplication>
#include <QMessageBox>
#include <QList>
#include <QSharedPointer>
#include <QProcess>
#include "settings/SettingsManager.h"
#include "AppRegistry.h"
#include "settings/ConnectionRecord.h"
#include "dialogs/ConnectionsDialog.h"
#include "Core.h"
#include "MainWindow.h"
#include "mongo/client/dbclient.h"
#include <QtScript>
#include <QScriptEngine>
#include <editors/PlainJavaScriptEditor.h>

using namespace Robomongo;

void insert( mongo::DBClientConnection & conn , const char * name , int num ) {
    mongo::BSONObjBuilder obj;
    obj.append( "name" , name );
    obj.append( "num" , num );
    conn.insert( "test.people" , obj.obj() );
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // eda 1!!!!
//    QScriptEngine *engine = new QScriptEngine();
//    QScriptValue ebati = engine->evaluate("(function() { return 2 + 56;})");
//    QString aga = ebati.toString();
//    int answer = ebati.call().toInt32(); // returns 58

    // eda 2!!!
//    QScriptEngine *engine = new QScriptEngine();
//    QScriptValue step1 = engine->evaluate("h = function() {};");
//    QScriptValue step2 = engine->evaluate("h.one = 23;");
//    QScriptValue step3 = engine->evaluate("h.two = h.one + 12;");
//    QScriptValue step4 = engine->evaluate("h.two");
//    int z = step4.toInt32(); // return 35

    // eda 3!!!
//    QScriptEngine *engine = new QScriptEngine();
//    QScriptValue step1 = engine->evaluate("Array.toString()");
//    QString result = step1.toString(); // returns: "function Array() { [native code] }"

    // eda 4!!!!
//    QScriptEngine *engine = new QScriptEngine();
//    QScriptValue ebati = engine->evaluate("(function() { return 2 + 56;})");
//    QString aga = ebati.toString();
//    int answer = ebati.call().toInt32(); // returns 58



    qRegisterMetaType<ConnectionRecordPtr>("ConnectionRecordPtr");

    QProcessEnvironment proc = QProcessEnvironment::systemEnvironment();
    QString str = proc.value("LD_LIBRARY_PATH");

    qDebug() << str;

    mongo::DBClientConnection conn;
    std::string errmsg;
    if ( ! conn.connect( std::string( "127.0.0.1:27017" ), errmsg ) ) {
        std::cout << "couldn't connect : " << errmsg << endl;
        return EXIT_FAILURE;
    }

    {
        // clean up old data from any previous tests
        mongo::BSONObjBuilder query;
        conn.remove( "test.people" , query.obj() );
    }

    insert( conn , "eliot" , 15 );
    insert( conn , "sara" , 23 );

    //QStringList environment = QProcess::systemEnvironment();



    AppRegistry::instance().settingsManager().save();

    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    QSize size(screenGeometry.width() - 450, screenGeometry.height() - 165);

    MainWindow win;
    win.resize(size);

    int x = (screenGeometry.width() - win.width()) / 2;
    int y = (screenGeometry.height() - win.height()) / 2;
    win.move(x, y);
    win.show();

//    QFile file("/home/dmitry/dev/tmp/prototype.js");
//    if(!file.open(QIODevice::ReadOnly));

//    QTextStream in(&file);
//    QString esprima = in.readAll();

//    RoboScintilla * robo = new RoboScintilla();
//    robo->setText(esprima);
////    robo->show();


//    QGraphicsScene scene;
//    QGraphicsProxyWidget *proxy = scene.addWidget(robo);
//    QTransform transform = proxy->transform();
//    transform.rotate(-45.0, Qt::YAxis);
//    transform.translate(200., 75.);
//    transform.scale(2.5, 2.);
//    proxy->setTransform(transform);
//    QGraphicsView view(&scene);
//    view.show();

//    ConnectionsDialog dialog(&AppRegistry::instance().settingsManager());
//    dialog.exec();

    //AppRegistry::instance().settingsManager().save();

//    MainWindow w;
//    w.show();

    return app.exec();
}
