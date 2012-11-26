#include "AppRegistry.h"
#include "Dispatcher.h"
#include "settings/SettingsManager.h"
#include "domain/App.h"

using namespace Robomongo;

AppRegistry::AppRegistry()
{
    _dispatcher.reset(new Dispatcher());
    _settingsManager.reset(new SettingsManager());
    _app.reset(new App(_dispatcher.get()));
}

AppRegistry::~AppRegistry()
{
}

