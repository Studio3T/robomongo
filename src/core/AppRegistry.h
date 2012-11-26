#ifndef APPREGISTRY_H
#define APPREGISTRY_H

#include "Core.h"

namespace Robomongo
{
    class SettingsManager;

    class AppRegistry
    {
    public:

        /**
         * @brief Returns single instance of AppRegistry
         */
        static AppRegistry &instance()
        {
            static AppRegistry _instance;
            return _instance;
        }

        SettingsManager &settingsManager() const { return *_settingsManager.get(); }
        App &app() const { return *_app.get(); }
        Dispatcher &dispatcher() const { return *_dispatcher.get(); }

    private:
        AppRegistry();
        ~AppRegistry(void);

        SettingsManager_ScopedPtr _settingsManager;
        AppScopedPtr _app;
        DispatcherScopedPtr _dispatcher;

        /**
         * Singleton support
         */
        AppRegistry(AppRegistry const &);       // To protect from copies of singleton
        void operator=(AppRegistry const &);    // To protect from copies of singleton
    };
}

#endif // APPREGISTRY_H
