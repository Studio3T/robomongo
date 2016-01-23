#include <QApplication>
#include <QDesktopWidget>

#include <locale.h>
#include <mongo/base/initializer.h>

#include "robomongo/gui/MainWindow.h"
#include "robomongo/gui/AppStyle.h"

int main(int argc, char *argv[], char** envp)
{
#ifdef Q_OS_WIN
    envp = NULL;
#endif
    mongo::runGlobalInitializersOrDie(argc, argv, envp);

    QApplication app(argc, argv);
    Robomongo::detail::initStyle();    
    setlocale(LC_NUMERIC, "C"); // Do not move this line

    QRect screenGeometry = QApplication::desktop()->availableGeometry();
    int horizontalMargin = (int)(screenGeometry.width() * 0.1);
    int verticalMargin = (int)(screenGeometry.height() * 0.1);
    QSize size(screenGeometry.width() - horizontalMargin,
               screenGeometry.height() - verticalMargin);

    Robomongo::MainWindow win;
    win.resize(size);
#if defined(Q_OS_MAC)
    win.setUnifiedTitleAndToolBarOnMac(true);
#endif
    int x = (screenGeometry.width() - win.width()) / 2;
    int y = (screenGeometry.height() - win.height()) / 2;
    win.move(x, y);
    win.show();

    return app.exec();
}
