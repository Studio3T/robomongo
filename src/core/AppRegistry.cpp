#include "AppRegistry.h"
#include "settings/SettingsManager.h"

using namespace Robomongo;

AppRegistry::AppRegistry()
{
    _settingsManager.reset(new SettingsManager());
}

AppRegistry::~AppRegistry()
{
}

