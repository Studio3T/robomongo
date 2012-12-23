#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QDir>
#include <QObject>
#include "ConnectionRecord.h"
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
         * @brief Cleanup owned objects
         */
        ~SettingsManager();

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
        void addConnection(ConnectionRecord *connection);

        /**
         * @brief Update connection
         */
        void updateConnection(ConnectionRecord *connection);

        /**
         * @brief Removes connection by index
         */
        void removeConnection(ConnectionRecord *connection);

        void reorderConnections(const QList<ConnectionRecord *> &connections);

        /**
         * @brief Returns list of connections
         */
        const QList<ConnectionRecord *> connections() const { return _connections; }

    signals:
        void connectionAdded(ConnectionRecord *connection);
        void connectionUpdated(ConnectionRecord *connection);
        void connectionRemoved(ConnectionRecord *connection);

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
        QList<ConnectionRecord *> _connections;
    };
}

#endif // SETTINGSMANAGER_H
