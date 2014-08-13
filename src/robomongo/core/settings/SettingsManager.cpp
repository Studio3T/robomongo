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
        _autocompletionMode(AutocompleteAll),
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

        if (!QDir().mkpath(_configDir)) {
            LOG_MSG("ERROR: Could not create settings path: " + _configDir, mongo::LL_ERROR);
            return false;
        }

        QFile f(_configPath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            LOG_MSG("ERROR: Could not write settings to: " + _configPath, mongo::LL_ERROR);
            return false;
        }

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
        _version = map.value("version").toString();

        // 2. Load UUID encoding
        int encoding = map.value("uuidEncoding").toInt();
        if (encoding > 3 || encoding < 0)
            encoding = 0;

        _uuidEncoding = (UUIDEncoding) encoding;


        // 3. Load view mode
        if (map.contains("viewMode")) {
            int viewMode = map.value("viewMode").toInt();
            if (viewMode > 2 || viewMode < 0)
                viewMode = Custom; // Default View Mode
            _viewMode = (ViewMode) viewMode;
        } else {
            _viewMode = Custom; // Default View Mode
        }

        _autoExpand = map.contains("autoExpand") ?
            map.value("autoExpand").toBool() : true;
        
        _autoExec = map.contains("autoExec") ?
            map.value("autoExec").toBool() : true;

        _lineNumbers = map.contains("lineNumbers") ?
            map.value("lineNumbers").toBool() : true;

        // 4. Load TimeZone
        int timeZone = map.value("timeZone").toInt();
        if (timeZone > 1 || timeZone < 0)
            timeZone = 0;

        _timeZone = (SupportedTimes) timeZone;
        _loadMongoRcJs = map.value("loadMongoRcJs").toBool();
        _disableConnectionShortcuts = map.value("disableConnectionShortcuts").toBool();

        // Load AutocompletionMode
        if (map.contains("autocompletionMode")) {
            int autocompletionMode = map.value("autocompletionMode").toInt();
            if (autocompletionMode < 0 || autocompletionMode > 2)
                autocompletionMode = AutocompleteAll; // Default Mode
            _autocompletionMode = (AutocompletionMode) autocompletionMode;
        } else {
            _autocompletionMode = AutocompleteAll; // Default Mode
        }

        // Load Batch Size
        _batchSize = map.value("batchSize").toInt();
        if (_batchSize == 0)
            _batchSize = 50;
        _currentStyle = map.value("style").toString();
        if (_currentStyle.isEmpty()) {
            _currentStyle = AppStyle::StyleName;
        }

        // 5. Load connections
        _connections.clear();

        QVariantList list = map.value("connections").toList();
        for (QVariantList::iterator it = list.begin(); it != list.end(); ++it) {
            ConnectionSettings *record = new ConnectionSettings((*it).toMap());
            _connections.push_back(record);
        }
        
        _toolbars = map.value("toolbars").toMap();
        ToolbarSettingsContainerType::const_iterator it = _toolbars.find("connect");
        if (_toolbars.end() == it) 
            _toolbars["connect"] = true;
        it = _toolbars.find("open_save");
        if (_toolbars.end() == it) 
            _toolbars["open_save"] = true;
        it = _toolbars.find("exec");
        if (_toolbars.end() == it) 
            _toolbars["exec"] = true;
        it = _toolbars.find("explorer");
        if (_toolbars.end() == it) 
            _toolbars["explorer"] = true;
        it = _toolbars.find("logs");
        if (_toolbars.end() == it) 
            _toolbars["logs"] = false;
    }

    /**
     * Save all settings to map.
     */
    QVariantMap SettingsManager::convertToMap() const
    {
        QVariantMap map;

        // 1. Save schema version
        map.insert("version", SchemaVersion);

        // 2. Save UUID encoding
        map.insert("uuidEncoding", _uuidEncoding);

        // 3. Save TimeZone encoding
        map.insert("timeZone", _timeZone);

        // 4. Save view mode
        map.insert("viewMode", _viewMode);
        map.insert("autoExpand", _autoExpand);
        map.insert("lineNumbers", _lineNumbers);

        // 5. Save Autocompletion mode
        map.insert("autocompletionMode", _autocompletionMode);

        // 6. Save loadInitJs
        map.insert("loadMongoRcJs", _loadMongoRcJs);

        // 7. Save disableConnectionShortcuts
        map.insert("disableConnectionShortcuts", _disableConnectionShortcuts);
        
        // 8. Save batchSize
        map.insert("batchSize", _batchSize);

        // 9. Save style
        map.insert("style", _currentStyle);

        // 10. Save connections
        QVariantList list;

        for (ConnectionSettingsContainerType::const_iterator it = _connections.begin(); it!=_connections.end(); ++it) {
            QVariantMap rm = (*it)->toVariant().toMap();
            list.append(rm);
        }

        map.insert("connections", list);
        
        map.insert("autoExec", _autoExec);
        
        map.insert("toolbars", _toolbars);

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
    
    void SettingsManager::setToolbarSettings(const QString toolbarName, const bool visible)
    {
        _toolbars[toolbarName] = visible;
    }

}
