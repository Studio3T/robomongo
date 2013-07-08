#pragma once
#include <QString>
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/Core.h"
namespace Robomongo
{
    class SettingsManager;

    class AppRegistry
    {
    public:

        QString version() const { return PROJECT_VERSION; }

        /**
         * @brief Returns single instance of AppRegistry
         */
        static AppRegistry &instance()
        {
            static AppRegistry _instance;
            return _instance;
        }

        SettingsManager *settingsManager() const { return _settingsManager.get(); }
        App *app() const { return _app.get(); }
        EventBus *bus() const { return _bus.get(); }
        KeyboardManager *keyboard() const { return _keyboard.get(); }

    private:
        AppRegistry();
        ~AppRegistry(void);

        EventBusScopedPtr _bus;
        SettingsManagerScopedPtr _settingsManager;
        AppScopedPtr _app;
        KeyboardManagerScopedPtr _keyboard;

        /**
         * Singleton support
         */
        AppRegistry(AppRegistry const &);       // To protect from copies of singleton
        void operator=(AppRegistry const &);    // To protect from copies of singleton
    };
}
