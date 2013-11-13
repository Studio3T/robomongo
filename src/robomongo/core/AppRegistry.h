#pragma once

#include "robomongo/core/Core.h"
#include "robomongo/core/utils/SingletonPattern.hpp"

namespace Robomongo
{
    class SettingsManager;

    class AppRegistry: public Patterns::LazySingleton<AppRegistry>
    {
        friend class Patterns::LazySingleton<AppRegistry>;
    public:

        SettingsManager *const settingsManager() const { return _settingsManager.get(); }
        App *const app() const { return _app.get(); }

    private:
        AppRegistry();
        ~AppRegistry();  

        const SettingsManagerScopedPtr _settingsManager;
        const AppScopedPtr _app;
    };
}
