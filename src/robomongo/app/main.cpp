#include <QApplication>
#include <QDesktopWidget>
#include <QtCore>

#include "robomongo/gui/MainWindow.h"
#include "robomongo/gui/AppStyle.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/utils/Logger.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Robomongo::detail::initStyle();
    setlocale(LC_NUMERIC, "C"); // do not move this line!!!
    
    Robomongo::AppRegistry::instance().application = &app;
    Robomongo::AppRegistry::instance().settingsManager()->switchTranslator(Robomongo::AppRegistry::instance().settingsManager()->currentTranslation(), true);
    
    QRect screenGeometry = QApplication::desktop()->availableGeometry();
    QSize size(screenGeometry.width() - 450, screenGeometry.height() - 165);

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
