#include "robomongo/core/settings/SettingsManager.h"

#include <QDir>
#include <QFile>
#include <QVariantList>
#include <QDirIterator>
#include <parser.h>
#include <serializer.h>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/utils/Logger.h"
#include "robomongo/core/utils/StdUtils.h"
#include "robomongo/gui/AppStyle.h"
#include "robomongo/core/AppRegistry.h"

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
        _qmPath = QCoreApplication::applicationDirPath() + "/../lib/translations";
        loadProvidedTranslations();
        LOG_MSG("SettingsManager initialized in " + _configPath, mongo::LL_INFO, false);
    }

    SettingsManager::~SettingsManager()
    {
        std::for_each(_connections.begin(), _connections.end(), stdutils::default_delete<ConnectionSettings *>());
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
        _version = map.value("version").toString();

        // 2. Load UUID encoding
        int encoding = map.value("uuidEncoding").toInt();
        if (encoding > 3 || encoding < 0)
            encoding = 0;

        _uuidEncoding = (UUIDEncoding) encoding;


        // 3. Load view mode
        if (map.contains("viewMode")) {
            int viewMode = map.value("viewMode").toInt();
            if (viewMode > 2 || encoding < 0)
                viewMode = Custom; // Default View Mode
            _viewMode = (ViewMode) viewMode;
        } else {
            _viewMode = Custom; // Default View Mode
        }

        _autoExpand = map.contains("autoExpand") ?
                map.value("autoExpand").toBool() : true;

        // 4. Load TimeZone
        int timeZone = map.value("timeZone").toInt();
        if (timeZone > 1 || timeZone < 0)
            timeZone = 0;

        _timeZone = (SupportedTimes) timeZone;
        _loadMongoRcJs = map.value("loadMongoRcJs").toBool();
        _disableConnectionShortcuts = map.value("disableConnectionShortcuts").toBool();

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

        _currentTranslation = map.value("translation").toString();
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

        // 5. Save loadInitJs
        map.insert("loadMongoRcJs", _loadMongoRcJs);

        // 6. Save disableConnectionShortcuts
        map.insert("disableConnectionShortcuts", _disableConnectionShortcuts);

        // 7. Save batchSize
        map.insert("batchSize", _batchSize);

        // 8. Save style
        map.insert("style", _currentStyle);

        // 9. Save connections
        QVariantList list;

        for (ConnectionSettingsContainerType::const_iterator it = _connections.begin(); it != _connections.end(); ++it) {
            QVariantMap rm = (*it)->toVariant().toMap();
            list.append(rm);
        }

        map.insert("connections", list);

        //10. Save translsation
        map.insert("translation", _currentTranslation);

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
        ConnectionSettingsContainerType::iterator it = std::find(_connections.begin(), _connections.end(), connection);
        if (it != _connections.end()) {
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

    void SettingsManager::setCurrentTranslation(const QString& translation)
    {
        _currentTranslation = _translations.contains(translation) == true ? translation : "";
    }

    void SettingsManager::loadProvidedTranslations()
    {
        QDirIterator qmIt(_qmPath, QStringList() << PROJECT_NAME_LOWERCASE"_*.qm", QDir::Files);
        _translations[""] = "locale";
        QFileInfo finfo;
        QTranslator translator;
        while (qmIt.hasNext()) {
            qmIt.next();
            finfo = qmIt.fileInfo();
            translator.load(finfo.baseName(), _qmPath);
            //: Native language name: "English" for English, "Русский" for Russian etc.
            QT_TR_NOOP("__LANGUAGE_NAME__");
            _translations[finfo.baseName()] = translator.translate("Robomongo::SettingsManager", "__LANGUAGE_NAME__");
        }
    }

    void SettingsManager::switchTranslator(const QString& translation, bool forced)
    {
        if (forced == true || translation != _currentTranslation) {
            QTranslator *tr = new QTranslator();
            QTranslator *trQt = new QTranslator();
            QString basename = translation.isEmpty() ? PROJECT_NAME_LOWERCASE"_" + QLocale::system().name() : translation;
            qApp->removeTranslator(_translator);            
            delete _translator; 
            _translator = NULL;
            if (tr->load(basename + ".qm", _qmPath)) {
                _translator = tr;
                qApp->installTranslator(_translator);
            }
            qApp->removeTranslator(_translatorQt);
            delete _translatorQt; 
            _translatorQt = NULL;
            if (trQt->load(basename.replace(PROJECT_NAME_LOWERCASE, "qt") + ".qm", _qmPath)) {
                _translatorQt = trQt;
                qApp->installTranslator(_translatorQt);
            }
            if (!forced) {
                setCurrentTranslation(translation);
                save();
                /** @TODO REMOVE */
                LOG_MSG("Translation switched to " + _currentTranslation, mongo::LL_INFO, false);
            }
        }
    }

}
