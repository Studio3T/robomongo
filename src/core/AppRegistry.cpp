#include "AppRegistry.h"
#include "Dispatcher.h"
#include "settings/SettingsManager.h"
#include "domain/MongoManager.h"

using namespace Robomongo;

AppRegistry::AppRegistry()
{
    _dispatcher.reset(new Dispatcher());
    _settingsManager.reset(new SettingsManager());
    _mongoManager.reset(new MongoManager(_dispatcher.get()));
}

AppRegistry::~AppRegistry()
{
}

