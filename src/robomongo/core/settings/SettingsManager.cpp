#include "robomongo/core/settings/SettingsManager.h"

#include <QDir>
#include <QFile>
#include <QVariantList>
#include <parser.h>
#include <serializer.h>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/utils/Logger.h"
#include "robomongo/core/utils/StdUtils.h"
#include "robomongo/gui/AppStyle.h"

#define VERSION_CONFIG "version"
#define UUIDENCODING "uuidEncoding"
#define TIMEZONE "timeZone"
#define VIEWMODE "viewMode"
#define AUTOEXPAND "autoExpand"
#define LOADMONGORCJS "loadMongoRcJs"
#define DISABLECONNECTIONSHORTCUTS "disableConnectionShortcuts"
#define BATCHSIZE "batchSize"
#define STYLE "style"
#define CONNECTIONS "connections"

namespace
{
        /**
         * @brief Version of schema
         */
        const QString SchemaVersion = "1.0";

         /**
         * @brief Config file absolute path
         *        (usually: /home/user/.config/robomongo/robomongo.json)
         */
        const QString _configPath = QString("%1/.config/"PROJECT_NAME_LOWERCASE"/"PROJECT_NAME_LOWERCASE".json").arg(QDir::homePath());

        /**
         * @brief Config file containing directory path
         *        (usually: /home/user/.config/robomongo)
         */
        const QString _configDir = QString("%1/.config/"PROJECT_NAME_LOWERCASE).arg(QDir::homePath());
}

namespace Robomongo
{
    /**
     * Creates SettingsManager for config file in default location
     * ~/.config/robomongo/robomongo.json
     */
    SettingsManager::SettingsManager() :
        _version(SchemaVersion),
        _uuidEncoding(DefaultEncoding),
        _timeZone(Utc),
        _viewMode(Robomongo::Tree),
        _batchSize(50),
        _disableConnectionShortcuts(false)
    {
        load();
        LOG_MSG("SettingsManager initialized in " + _configPath, mongo::LL_INFO, false);
    }

    SettingsManager::~SettingsManager()
    {
        std::for_each(_connections.begin(),_connections.end(),stdutils::default_delete<ConnectionSettings *>());
    }

    /**
     * Load settings from config file.
     * @return true if success, false otherwise
     */
    bool SettingsManager::load()
    {
        if (!QFile::exists(_configPath))
            return false;

        QFile f(_configPath);
        if (!f.open(QIODevice::ReadOnly))
            return false;

        bool ok;
        QJson::Parser parser;
        QVariantMap map = parser.parse(f.readAll(), &ok).toMap();
        if (!ok)
            return false;

        loadFromMap(map);

        return true;
    }

    /**
     * Saves all settings to config file.
     * @return true if success, false otherwise
     */
    bool SettingsManager::save()
    {
        QVariantMap map = convertToMap();

        if (!QDir().mkpath(_configDir))
            return false;

        QFile f(_configPath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;

        bool ok;
        QJson::Serializer s;
        s.setIndentMode(QJson::IndentFull);
        s.serialize(map, &f, &ok);

        LOG_MSG("Settings saved to: " + _configPath, mongo::LL_INFO);

        return ok;
    }

    /**
     * Load settings from the map. Existings settings will be overwritten.
     */
    void SettingsManager::loadFromMap(QVariantMap &map)
    {
        // 1. Load version
        _version = map.value(VERSION_CONFIG).toString();

        // 2. Load UUID encoding
        int encoding = map.value(UUIDENCODING).toInt();
        if (encoding > 3 || encoding < 0)
            encoding = 0;

        _uuidEncoding = (UUIDEncoding) encoding;


        // 3. Load view mode
        if (map.contains(VIEWMODE)) {
            int viewMode = map.value(VIEWMODE).toInt();
            if (viewMode > 2 || encoding < 0)
                viewMode = Custom; // Default View Mode
            _viewMode = (ViewMode) viewMode;
        } else {
            _viewMode = Custom; // Default View Mode
        }
        _autoExpand = map.value(AUTOEXPAND).toBool();

        // 4. Load TimeZone
        int timeZone = map.value(TIMEZONE).toInt();
        if (timeZone > 1 || timeZone < 0)
            timeZone = 0;

        _timeZone = (SupportedTimes) timeZone;
        _loadMongoRcJs = map.value(LOADMONGORCJS).toBool();
        _disableConnectionShortcuts = map.value(DISABLECONNECTIONSHORTCUTS).toBool();

        // Load Batch Size
        _batchSize = map.value(BATCHSIZE).toInt();
        if (_batchSize == 0)
            _batchSize = 50;
        _currentStyle = map.value(STYLE).toString();
        if (_currentStyle.isEmpty()) {
            _currentStyle = AppStyle::StyleName;
        }

        // 5. Load connections
        _connections.clear();

        QVariantList list = map.value(CONNECTIONS).toList();
        for (QVariantList::iterator it = list.begin(); it != list.end(); ++it) {
            ConnectionSettings *record = new ConnectionSettings((*it).toMap());
            _connections.push_back(record);
        }
    }

    /**
     * Save all settings to map.
     */
    QVariantMap SettingsManager::convertToMap() const
    {
        QVariantMap map;

        // 1. Save schema version
        map.insert(VERSION_CONFIG, SchemaVersion);

        // 2. Save UUID encoding
        map.insert(UUIDENCODING, _uuidEncoding);

        // 3. Save TimeZone encoding
        map.insert(TIMEZONE, _timeZone);

        // 4. Save view mode
        map.insert(VIEWMODE, _viewMode);
        map.insert(AUTOEXPAND, _autoExpand);

        // 5. Save loadInitJs
        map.insert(LOADMONGORCJS, _loadMongoRcJs);

        // 6. Save disableConnectionShortcuts
        map.insert(DISABLECONNECTIONSHORTCUTS, _disableConnectionShortcuts);
        
        // 7. Save batchSize
        map.insert(BATCHSIZE, _batchSize);

        // 8. Save style
        map.insert(STYLE, _currentStyle);

        // 9. Save connections
        QVariantList list;

        for (ConnectionSettingsContainerType::const_iterator it = _connections.begin(); it!=_connections.end(); ++it) {
            QVariantMap rm = (*it)->toVariant().toMap();
            list.append(rm);
        }

        map.insert(CONNECTIONS, list);

        return map;
    }

    /**
     * Adds connection to the end of list
     */
    void SettingsManager::addConnection(ConnectionSettings *connection)
    {
        _connections.push_back(connection);
    }

    /**
     * Removes connection by index
     */
    void SettingsManager::removeConnection(ConnectionSettings *connection)
    {
        ConnectionSettingsContainerType::iterator it = std::find(_connections.begin(),_connections.end(),connection);
        if (it!=_connections.end()) {
            _connections.erase(it);
            delete connection;
        }
    }

    void SettingsManager::setCurrentStyle(const QString& style)
    {
        _currentStyle = style;
    }

    void SettingsManager::reorderConnections(const ConnectionSettingsContainerType &connections)
    {
        _connections = connections;
    }

}
