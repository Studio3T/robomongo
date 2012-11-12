#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QDir>

#include "ConnectionRecord.h"
#include "boost/ptr_container/ptr_vector.hpp"

namespace Robomongo
{
    class SettingsManager
    {

    public:
        SettingsManager();
        ~SettingsManager();

        bool load();
        bool save();


        void addConnection(ConnectionRecord * connection);
        void removeConnection(int index);

    private:
        QString _configPath;
        QString _configDir;

        QString _version;
        boost::ptr_vector<ConnectionRecord> _connections;
    };
}

#endif // SETTINGSMANAGER_H
