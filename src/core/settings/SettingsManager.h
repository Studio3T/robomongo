#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QDir>
#include <QObject>
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
    class SettingsManager : public QObject
    {
        Q_OBJECT

    public:
        /**
         * Creates SettingsManager for config file in default location
         * ~/.config/robomongo/robomongo.json
         */
        SettingsManager(QObject *parent=0);

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
        void addConnection(const ConnectionRecord &connection);

        /**
         * Update connection
         */
        void updateConnection(const ConnectionRecord &connection);

        /**
         * Removes connection by index
         */
        void removeConnection(const ConnectionRecord &connection);

        /**
         * Returns list of connections
         */
        const QList<ConnectionRecord> connections() const { return _connections; }

    signals:
        void connectionAdded(const ConnectionRecord &connection);
        void connectionUpdated(const ConnectionRecord &connection);
        void connectionRemoved(const ConnectionRecord &connection);

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
        QList<ConnectionRecord> _connections;
    };
}

#endif // SETTINGSMANAGER_H
