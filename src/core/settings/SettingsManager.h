#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QDir>
#include "ConnectionRecord.h"
#include "boost/ptr_container/ptr_vector.hpp"
#include "Core.h"


namespace Robomongo
{

    /**
     * SettingsManager gives you access to all settings, that is used
     * by Robomongo. It can load() and save() them. Config file usually
     * located here: ~/.config/robomongo/robomongo.json
     *
     * You can access this manager via:
     * AppRegistry::instance().settingsManager()
     *
     * @threadsafe no
     */
    class SettingsManager
    {
    public:
        /**
         * Creates SettingsManager for config file in default location
         * ~/.config/robomongo/robomongo.json
         */
        SettingsManager();

        /**
         * Load settings from config file.
         * @return true if success, false otherwise
         */
        bool load();

        /**
         * Saves all settings to config file.
         * @return true if success, false otherwise
         */
        bool save();

        /**
         * Adds connection to the end of list
         */
        void addConnection(ConnectionRecord *connection);

        /**
         * Removes connection by index
         */
        void removeConnection(int index);

        /**
         * Returns list of connections
         */
        const ConnectionRecord_PtrVector connections() const { return _connections; }

    private:

        /**
         * Load settings from the map. Existings settings will be overwritten.
         */
        void loadFromMap(QVariantMap &map);

        /**
         * Save all settings to map.
         */
        QVariantMap convertToMap() const;

        /**
         * Config file absolute path
         * (usually: /home/user/.config/robomongo/robomongo.json)
         */
        QString _configPath;

        /**
         * Config file containing directory path
         * (usually: /home/user/.config/robomongo)
         */
        QString _configDir;

        /**
         * Version of app
         */
        QString _version;

        /**
         * List of connections
         */
        ConnectionRecord_PtrVector _connections;
    };
}

#endif // SETTINGSMANAGER_H
