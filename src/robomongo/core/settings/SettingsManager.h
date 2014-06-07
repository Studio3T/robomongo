#pragma once

#include <QString>
#include <QVariantMap>
#include <vector>
#include "robomongo/core/Enums.h"

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
    class SettingsManager
    {
    public:
        typedef std::vector<ConnectionSettings *> ConnectionSettingsContainerType;
        typedef QMap<QString, QVariant> ToolbarSettingsContainerType;
        /**
         * @brief Creates SettingsManager for config file in default location
         *        (usually ~/.config/robomongo/robomongo.json)
         */
        SettingsManager();

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
         * @brief Removes connection by index
         */
        void removeConnection(ConnectionSettings *connection);

        void reorderConnections(const ConnectionSettingsContainerType &connections);

        void setToolbarSettings(QString toolbarName, bool visible);
        
        /**
         * @brief Returns list of connections
         */
        ConnectionSettingsContainerType connections() const { return _connections; }
        
        ToolbarSettingsContainerType toolbars() const { return _toolbars; }

        void setUuidEncoding(UUIDEncoding encoding) { _uuidEncoding = encoding; }
        UUIDEncoding uuidEncoding() const { return _uuidEncoding; }

        void setTimeZone(SupportedTimes timeZ) { _timeZone = timeZ; }
        SupportedTimes timeZone() const { return _timeZone;}

        void setViewMode(ViewMode viewMode) { _viewMode = viewMode; }
        ViewMode viewMode() const { return _viewMode; }

        void setAutocompletionMode(AutocompletionMode mode) { _autocompletionMode = mode; }
        AutocompletionMode autocompletionMode() const { return _autocompletionMode; }

        void setAutoExpand(bool isExpand) { _autoExpand = isExpand; }
        bool autoExpand() const { return _autoExpand; }
        
        void setAutoExec(bool isAutoExec) { _autoExec = isAutoExec; }
        bool autoExec() const { return _autoExec; }

        void setLineNumbers(bool showLineNumbers) { _lineNumbers = showLineNumbers; }
        bool lineNumbers() const { return _lineNumbers; }

        void setLoadMongoRcJs(bool isLoadJs) { _loadMongoRcJs = isLoadJs; }
        bool loadMongoRcJs() const { return _loadMongoRcJs; }

        void setDisableConnectionShortcuts(bool isDisable) { _disableConnectionShortcuts = isDisable; }
        bool disableConnectionShortcuts() const { return _disableConnectionShortcuts; }

        void setBatchSize(int batchSize) { _batchSize = batchSize; }
        int batchSize() const { return _batchSize; }

        QString currentStyle() const {return _currentStyle; }
        void setCurrentStyle(const QString& style);


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
        AutocompletionMode _autocompletionMode;
        bool _loadMongoRcJs;
        bool _autoExpand;
        bool _autoExec;
        bool _lineNumbers;
        bool _disableConnectionShortcuts;
        int _batchSize;
        QString _currentStyle;
        /**
         * @brief List of connections
         */
        ConnectionSettingsContainerType _connections;
        ToolbarSettingsContainerType _toolbars;
    };
}
