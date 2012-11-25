#include "AppRegistry.h"
#include "Dispatcher.h"
#include "settings/SettingsManager.h"
#include "mongodb/MongoManager.h"

using namespace Robomongo;

AppRegistry::AppRegistry()
{
    _settingsManager.reset(new SettingsManager());
    _mongoManager.reset(new MongoManager());
    _dispatcher.reset(new Dispatcher());
}

AppRegistry::~AppRegistry()
{
}

