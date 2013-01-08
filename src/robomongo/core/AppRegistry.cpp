#include "AppRegistry.h"
#include "EventBus.h"
#include "settings/SettingsManager.h"
#include "domain/App.h"
#include "KeyboardManager.h"

using namespace Robomongo;

AppRegistry::AppRegistry() :
    _bus(new EventBus()),
    _settingsManager(new SettingsManager()),
    _app(new App(_bus.get())),
    _keyboard(new KeyboardManager())
{
}

AppRegistry::~AppRegistry()
{
}

