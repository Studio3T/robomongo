#include "robomongo/core/AppRegistry.h"

#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/domain/App.h"

namespace Robomongo
{
    AppRegistry::AppRegistry() :
        _settingsManager(new SettingsManager()),
        _app(new App())
    {
    }

    AppRegistry::~AppRegistry()
    {   
    }
}

