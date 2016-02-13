#include <QApplication>
#include <QDesktopWidget>

#include <locale.h>

// Header "mongo/util/net/sock" is needed for mongo::enableIPv6()
// Header "mongo/platform/basic" is required by "sock.h" under Windows
#include <mongo/platform/basic.h>
#include <mongo/util/net/sock.h>

#include <mongo/base/initializer.h>

#include "robomongo/gui/MainWindow.h"
#include "robomongo/gui/AppStyle.h"

int main(int argc, char *argv[], char** envp)
{
    // Please check, do we really need envp for other OSes?
#ifdef Q_OS_WIN
    envp = NULL;
#endif

    // Support for IPv6 is disabled by default. Enable it.
    mongo::enableIPv6(true);

    // Initialization routine for MongoDB shell
    mongo::runGlobalInitializersOrDie(argc, argv, envp);

    // Initialize Qt application
    QApplication app(argc, argv);

    // On Unix/Linux Qt is configured to use the system locale settings by default.
    // This can cause a conflict when using POSIX functions, for instance, when
    // converting between data types such as floats and strings, since the notation
    // may differ between locales. To get around this problem, call the POSIX
    // function setlocale(LC_NUMERIC,"C") right after initializing QApplication or
    // QCoreApplication to reset the locale that is used for number formatting to "C"-locale.
    // (http://doc.qt.io/qt-5/qcoreapplication.html#locale-settings)
    setlocale(LC_NUMERIC, "C");

    // Init GUI style
    Robomongo::AppStyleUtils::initStyle();

    // Application main window
    Robomongo::MainWindow win;

    // Resize main window. We are trying to keep it "almost" maximized.
    QRect screenGeometry = QApplication::desktop()->availableGeometry();
    int horizontalMargin = (int)(screenGeometry.width() * 0.1);
    int verticalMargin = (int)(screenGeometry.height() * 0.1);
    int width = screenGeometry.width() - horizontalMargin;
    int height = screenGeometry.height() - verticalMargin;
    win.resize(QSize(width, height));

    // Center main window
    int x = (screenGeometry.width() - win.width()) / 2;
    int y = (screenGeometry.height() - win.height()) / 2;
    win.move(x, y);

    // And, finally, show it
    win.show();

    return app.exec();
}
