#ifndef APPREGISTRY_H
#define APPREGISTRY_H

//#include "Core.h"

namespace Robomongo
{
    class SettingsManager;

    class AppRegistry
    {
    public:
        ~AppRegistry(void);

        static AppRegistry & instance()
        {
            static AppRegistry _instance;
            return _instance;
        }

        SettingsManager * settingsManager() { return _settingsManager; }

    private:
        AppRegistry();
        SettingsManager * _settingsManager;

        /*
        ** Singleton support
        */
        AppRegistry(AppRegistry const &);       // To protect from copies of singleton
        void operator=(AppRegistry const &);	// To protect from copies of singleton
    };
}

#endif // APPREGISTRY_H
