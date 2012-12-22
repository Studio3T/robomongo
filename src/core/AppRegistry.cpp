#include "AppRegistry.h"
#include "EventBus.h"
#include "settings/SettingsManager.h"
#include "domain/App.h"

using namespace Robomongo;

AppRegistry::AppRegistry()
{
    _bus.reset(new EventBus());
    _settingsManager.reset(new SettingsManager());
    _app.reset(new App(_bus.get()));
}

AppRegistry::~AppRegistry()
{
}

