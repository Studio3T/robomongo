#pragma once
#include "robomongo/core/Core.h"
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

        SettingsManager *const settingsManager() const { return _settingsManager.get(); }
        App *const app() const { return _app.get(); }
        EventBus *const bus() const { return _bus.get(); }
    private:
        AppRegistry();
        ~AppRegistry();
        /**
        * Singleton support
        */
        AppRegistry(AppRegistry const &);       // To protect from copies of singleton
        void operator=(AppRegistry const &);    // To protect from copies of singleton    

        EventBusScopedPtr _bus;
        SettingsManagerScopedPtr _settingsManager;
        AppScopedPtr _app;
    };
}
