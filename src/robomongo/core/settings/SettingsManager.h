#pragma once

#include <QObject>

#include "robomongo/gui/ViewMode.h"
#include "robomongo/core/domain/Enums.h"

namespace Robomongo
{
    class ConnectionSettings;
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
        SettingsManager(QObject *parent = NULL);

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
         * @brief Adds connection to the end of list.
         * Connection now will be owned by SettingsManager.
         */
        void addConnection(ConnectionSettings *connection);

        /**
         * @brief Update connection
         */
        void updateConnection(ConnectionSettings *connection);

        /**
         * @brief Removes connection by index
         */
        void removeConnection(ConnectionSettings *connection);

        void reorderConnections(const QList<ConnectionSettings *> &connections);

        /**
         * @brief Returns list of connections
         */
        const QList<ConnectionSettings *> connections() const { return _connections; }

        void setUuidEncoding(UUIDEncoding encoding) { _uuidEncoding = encoding; }
        void setTimeZone(SupportedTimes timeZ) { _timeZone = timeZ; }
        UUIDEncoding uuidEncoding() { return _uuidEncoding; }
        SupportedTimes timeZone() { return _timeZone;}

        void setViewMode(ViewMode viewMode) { _viewMode = viewMode; }
        ViewMode viewMode() { return _viewMode; }

        void setLoadInitJs(bool isLoadJs) { _loadInitJs = isLoadJs;}
        bool loadInitJs() const {return _loadInitJs;}
    signals:
        void connectionAdded(ConnectionSettings *connection);
        void connectionUpdated(ConnectionSettings *connection);
        void connectionRemoved(ConnectionSettings *connection);

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
         * @brief Version of settings schema currently loaded
         */
        QString _version;

        /**
         * @brief UUID encoding
         */
        UUIDEncoding _uuidEncoding;
        SupportedTimes _timeZone;
        /**
         * @brief view mode
         */
        ViewMode _viewMode;
        bool _loadInitJs;
        /**
         * @brief List of connections
         */
        QList<ConnectionSettings *> _connections;
    };
}
