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

    QString translation = Robomongo::AppRegistry::instance().settingsManager()->currentTranslation();
    QString qmPath = Robomongo::AppRegistry::instance().settingsManager()->getQmPath();
    
    if (translation.isEmpty())
        translation = PROJECT_NAME_LOWERCASE"_" + QLocale::system().name();
    Robomongo::LOG_MSG("Attempt to load: " + translation + " from " + qmPath, mongo::LL_INFO);
    QTranslator translator;
    if (!translator.load(translation, qmPath))
        Robomongo::LOG_MSG("Translation loading failed", mongo::LL_INFO);
    if (!app.installTranslator(&translator))
        Robomongo::LOG_MSG("Translator installation failed", mongo::LL_INFO);
    
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
