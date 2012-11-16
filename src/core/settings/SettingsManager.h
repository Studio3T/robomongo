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
     * @brief SettingsManager gives you access to all settings, that is used
     *        by Robomongo. It can load() and save() them. Config file usually
     *        located here: ~/.config/robomongo/robomongo.json
     *
     *        You can access this manager via:
     *        AppRegistry::instance().settingsManager()
     *
     * @threadsafe no
     */
    class SettingsManager : public QObject
    {
        Q_OBJECT

    public:
        /**
         * @brief Creates SettingsManager for config file in default location
         *        (usually ~/.config/robomongo/robomongo.json)
         */
        SettingsManager(QObject *parent=0);

        /**
         * @brief Load settings from config file.
         * @return true if success, false otherwise
         */
        bool load();

        /**
         * @brief Saves all settings to config file.
         * @return true if success, false otherwise
         */
        bool save();

        /**
         * @brief Adds connection to the end of list
         */
        void addConnection(const ConnectionRecordShared &connection);

        /**
         * @brief Update connection
         */
        void updateConnection(const ConnectionRecordShared &connection);

        /**
         * @brief Removes connection by index
         */
        void removeConnection(const ConnectionRecordShared &connection);

        /**
         * @brief Returns list of connections
         */
        const QList<ConnectionRecordShared> connections() const { return _connections; }

    signals:
        void connectionAdded(const ConnectionRecordShared &connection);
        void connectionUpdated(const ConnectionRecordShared &connection);
        void connectionRemoved(const ConnectionRecordShared &connection);

    private:

        /**
         * @brief Load settings from the map. Existings settings will be overwritten.
         */
        void loadFromMap(QVariantMap &map);

        /**
         * @brief Save all settings to map.
         */
        QVariantMap convertToMap() const;

        /**
         * @brief Config file absolute path
         *        (usually: /home/user/.config/robomongo/robomongo.json)
         */
        QString _configPath;

        /**
         * @brief Config file containing directory path
         *        (usually: /home/user/.config/robomongo)
         */
        QString _configDir;

        /**
         * @brief Version of app
         */
        QString _version;

        /**
         * @brief List of connections
         */
        QList<ConnectionRecordShared> _connections;
    };
}

#endif // SETTINGSMANAGER_H
