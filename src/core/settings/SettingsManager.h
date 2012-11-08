#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include "ConnectionRecord.h"

namespace Robomongo
{
    class SettingsManager
    {
    public:
        SettingsManager();

        void addConnection(const ConnectionRecord & connection);
    };
}

#endif // SETTINGSMANAGER_H
