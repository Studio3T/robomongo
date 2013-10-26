#include <QApplication>
#include <QDesktopWidget>
#include <QtCore>

#include "robomongo/core/utils/Logger.h"

#include "robomongo/gui/MainWindow.h"
#include "robomongo/gui/AppStyle.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Robomongo::detail::initStyle();    
    setlocale(LC_NUMERIC,"C"); // do not move this line!!!
    
    /** @todo clean here */
    QTranslator translator;
    Robomongo::LOG_MSG(QDir::currentPath(), mongo::LL_INFO);
    Robomongo::LOG_MSG(QCoreApplication::applicationDirPath(), mongo::LL_INFO);
    QString localization = PROJECT_NAME_LOWERCASE"_" + QLocale::system().name();
    Robomongo::LOG_MSG("Attempt to load: " + localization, mongo::LL_INFO);
    bool tr_loaded = translator.load(localization, QCoreApplication::applicationDirPath()+"/../lib/translations");
    if (tr_loaded)
        Robomongo::LOG_MSG("Translator loaded", mongo::LL_INFO);
    bool install_res = app.installTranslator(&translator);
    if (install_res)
        Robomongo::LOG_MSG("Translator installed", mongo::LL_INFO);

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
