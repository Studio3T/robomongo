#include <QApplication>
#include <QDesktopWidget>

#include <locale.h>

// Header "mongo/util/net/sock" is needed for mongo::enableIPv6()
// Header "mongo/platform/basic" is required by "sock.h" under Windows
#include <mongo/platform/basic.h>
#include <mongo/util/net/socket_utils.h>
#include <mongo/base/initializer.h>
#include <mongo/util/net/ssl_options.h>
#include <mongo/db/service_context.h>
#include <mongo/transport/transport_layer_asio.h>
#include <mongo/shell/shell_options.h>
#include <mongo/db/storage/storage_engine_init.h>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/utils/Logger.h"       
#include "robomongo/gui/MainWindow.h"
#include "robomongo/gui/AppStyle.h"
#include "robomongo/gui/dialogs/EulaDialog.h"
#include "robomongo/ssh/ssh.h"
#include "robomongo/utils/RoboCrypt.h"       

int main(int argc, char *argv[], char** envp)
{
    if (rbm_ssh_init()) 
        return 1;

    // Please check, do we really need envp for other OSes?
#ifdef Q_OS_WIN
    envp = NULL;
#endif

    // Support for IPv6 is disabled by default. Enable it.
    mongo::enableIPv6(true);

    // Perform SSL-enabled mongo initialization
    mongo::sslGlobalParams.sslMode.store(mongo::SSLParams::SSLMode_allowSSL);

    // Cross Platform High DPI support - Qt 5.7
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    // Initialization routine for MongoDB shell
    mongo::runGlobalInitializersOrDie(argc, argv, envp);
    mongo::setGlobalServiceContext(mongo::ServiceContext::make());
    // Todo from mongo repo: This should use a TransportLayerManager or TransportLayerFactory
    auto serviceContext = mongo::getGlobalServiceContext();
    mongo::transport::TransportLayerASIO::Options opts;
    opts.enableIPv6 = mongo::shellGlobalParams.enableIPv6;
    opts.mode = mongo::transport::TransportLayerASIO::Options::kEgress;
    serviceContext->setTransportLayer(
        std::make_unique<mongo::transport::TransportLayerASIO>(opts, nullptr));
    auto tlPtr = serviceContext->getTransportLayer();
    uassertStatusOK(tlPtr->setup());
    uassertStatusOK(tlPtr->start());    

    // Initialize Qt application
    QApplication app(argc, argv);

    // On Unix/Linux Qt is configured to use the system locale settings by default.
    // This can cause a conflict when using POSIX functions, for instance, when
    // converting between data types such as floats and strings, since the notation
    // may differ between locales. To get around this problem, call the POSIX
    // function setlocale(LC_NUMERIC, "C") right after initializing QApplication or
    // QCoreApplication to reset the locale that is used for number formatting to "C"-locale.
    // (http://doc.qt.io/qt-5/qcoreapplication.html#locale-settings)
    setlocale(LC_NUMERIC, "C");

#ifdef Q_OS_MAC
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    auto const& settingsManager = Robomongo::AppRegistry::instance().settingsManager();    
    // Disable HTTPS and do not show EULA form if program exited abnormally
    bool showEulaDialogForm = true;
    if (!settingsManager->programExitedNormally()) {
        showEulaDialogForm = false;
        settingsManager->setUseHttps(false);
        settingsManager->save();
    }
    
    if (!settingsManager->useHttps())
        showEulaDialogForm = false;
    
    // EULA License Agreement
    if (!settingsManager->acceptedEulaVersions().contains(PROJECT_VERSION)) {
        Robomongo::EulaDialog eulaDialog(showEulaDialogForm);
        settingsManager->setProgramExitedNormally(false);
        settingsManager->save();
        int const result = eulaDialog.exec();
        settingsManager->setProgramExitedNormally(true);
        settingsManager->save();
        if (QDialog::Rejected == result) {
            rbm_ssh_cleanup();
            return 1;
        }
        // EULA accepted
        settingsManager->addAcceptedEulaVersion(PROJECT_VERSION);
        settingsManager->save();
    }  

    // Init GUI style
    Robomongo::AppStyleUtils::initStyle();

    // To be set true at normal program exit
    settingsManager->setProgramExitedNormally(false);
    settingsManager->save();

    // Application main window
    Robomongo::MainWindow mainWindow;
    mainWindow.show();

    for(auto const& msgAndSeverity : Robomongo::RoboCrypt::roboCryptLogs())
        Robomongo::LOG_MSG(msgAndSeverity.first, msgAndSeverity.second);

    int rc = app.exec();
    rbm_ssh_cleanup();
    return rc;
}
