#include "robomongo/core/AppRegistry.h"

#include "robomongo/core/EventBus.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/domain/App.h"

namespace Robomongo
{
    AppRegistry::AppRegistry() :
        _bus(new EventBus()),
        _settingsManager(new SettingsManager()),
        _app(new App(_bus.get()))
    {
    }

    AppRegistry::~AppRegistry()
    {   
    }
}

