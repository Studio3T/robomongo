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
        MongoManager &mongoManager() const { return *_mongoManager.get(); }

    private:
        AppRegistry();
        ~AppRegistry(void);

        SettingsManager_ScopedPtr _settingsManager;
        MongoManagerScopedPtr _mongoManager;

        /**
         * Singleton support
         */
        AppRegistry(AppRegistry const &);       // To protect from copies of singleton
        void operator=(AppRegistry const &);    // To protect from copies of singleton
    };
}

#endif // APPREGISTRY_H
