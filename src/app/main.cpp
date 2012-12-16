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
#include <editors/PlainJavaScriptEditor.h>
#include <GuiRegistry.h>

using namespace Robomongo;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Add ./plugins directory to library path
    QString plugins = QString("%1%2plugins")
            .arg(QApplication::applicationDirPath())
            .arg(QDir::separator());

    QApplication::addLibraryPath(plugins);

    qRegisterMetaType<ConnectionRecordPtr>("ConnectionRecordPtr");

    QProcessEnvironment proc = QProcessEnvironment::systemEnvironment();
    QString str = proc.value("LD_LIBRARY_PATH");

    qDebug() << str;

    AppRegistry::instance().settingsManager().save();

    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    QSize size(screenGeometry.width() - 450, screenGeometry.height() - 165);

    MainWindow win;
    win.resize(size);

    int x = (screenGeometry.width() - win.width()) / 2;
    int y = (screenGeometry.height() - win.height()) / 2;
    win.move(x, y);
    win.show();

    return app.exec();
}
