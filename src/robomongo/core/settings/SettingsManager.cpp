#include "robomongo/core/settings/SettingsManager.h"

#include <QDir>
#include <QFile>
#include <QVariantList>
#include <QUuid>
#include <QJsonArray>
#include <QXmlStreamReader>
#include <QDirIterator>

#include <parser.h>
#include <serializer.h>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/settings/SshSettings.h"
#include "robomongo/core/settings/SslSettings.h"
#include "robomongo/core/utils/Logger.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/utils/StdUtils.h"
#include "robomongo/gui/AppStyle.h"
#include "robomongo/utils/common.h"
#include "robomongo/utils/qzip/qzipreader_p.h"
#include "robomongo/utils/RoboCrypt.h"

namespace Robomongo
{
    // 3T config files
    auto const Studio3T_PropertiesDat {
        QString("%1/.3T/studio-3t/properties.dat").arg(QDir::homePath())
    };
    auto const DataMongodb_PropertiesDat { 
        QString("%1/.3T/data-man-mongodb/properties.dat").arg(QDir::homePath())
    };
    auto const MongoChefPro_PropertiesDat {
        QString("%1/.3T/mongochef-pro/properties.dat").arg(QDir::homePath())
    };
    auto const MongoChefEnt_PropertiesDat {
        QString("%1/.3T/mongochef-enterprise/properties.dat").arg(QDir::homePath())
    };

    const std::vector<std::pair<QString, QString>> S_3T_ZipFile_And_ConfigFile_List
    {
        { Studio3T_PropertiesDat, "Studio3T.properties" },
        { DataMongodb_PropertiesDat, "3T.data-man-mongodb.properties" },
        { MongoChefPro_PropertiesDat, "3T.mongochef-pro.properties" },
        { MongoChefEnt_PropertiesDat, "3T.mongochef-enterprise.properties" }
    };

    // Extract zipFile and find the value of "anonymousID" field in propFile
    QString extractAnonymousIDFromZip(QString const& zipFile, QString const& propfile);

    // Extract "anonymousID" from a config file
    QString extractAnonymousID(QString const& configFile);

    /**
        * @brief Version of schema
    */
    const QString SchemaVersion = "2.0";

    const auto CONFIG_FILE_0_8_5 {
        QString("%1/.config/robomongo/robomongo.json").arg(QDir::homePath())
    };
    const auto CONFIG_FILE_1_0_RC1 {
        QString("%1/.config/robomongo/1.0/robomongo.json").arg(QDir::homePath())
    };
    const auto CONFIG_FILE_1_1_0_BETA {
        QString("%1/.3T/robomongo/1.1.0-Beta/robomongo.json").arg(QDir::homePath())
    };

    /**
    * @brief Robomongo config. files of old versions
    */
    // Important: In order to import connections from a version, config. file path must
    //            be defined and placed into the vector initializer list below in order.
    std::vector<QString> const SettingsManager::_configFilesOfOldVersions
    {
        QString("%1/.3T/robo-3t/1.4.3/robo3t.json").arg(QDir::homePath()), // CONFIG_FILE_1_4_3
        QString("%1/.3T/robo-3t/1.4.2/robo3t.json").arg(QDir::homePath()), // CONFIG_FILE_1_4_2
        QString("%1/.3T/robo-3t/1.4.1/robo3t.json").arg(QDir::homePath()), // CONFIG_FILE_1_4_1
        QString("%1/.3T/robo-3t/1.4.0/robo3t.json").arg(QDir::homePath()), // CONFIG_FILE_1_4_0
        QString("%1/.3T/robo-3t/1.3.1/robo3t.json").arg(QDir::homePath()), // CONFIG_FILE_1_3_1
        QString("%1/.3T/robo-3t/1.3.0/robo3t.json").arg(QDir::homePath()), // CONFIG_FILE_1_3_0_BETA
        QString("%1/.3T/robo-3t/1.2.1/robo3t.json").arg(QDir::homePath()), // CONFIG_FILE_1_2_1
        QString("%1/.3T/robo-3t/1.2.0/robo3t.json").arg(QDir::homePath()), // CONFIG_FILE_1_2_0_BETA
        QString("%1/.3T/robo-3t/1.1.1/robo3t.json").arg(QDir::homePath()), // CONFIG_FILE_1_1_1
        CONFIG_FILE_1_1_0_BETA,                                            // CONFIG_FILE_1_1_0_BETA 
        QString("%1/.3T/robomongo/1.0.0/robomongo.json").arg(QDir::homePath()),   // CONFIG_FILE_1_0_0     
        CONFIG_FILE_1_0_RC1,                                               // CONFIG_FILE_1_0_RC1
        QString("%1/.config/robomongo/0.9/robomongo.json").arg(QDir::homePath()), // CONFIG_FILE_0_9
        CONFIG_FILE_0_8_5                                                  // CONFIG_FILE_0_8_5
    };

    std::vector<ConnectionSettings*>  SettingsManager::_connections;
    
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
        _loadMongoRcJs(false),
        _minimizeToTray(false),
        _lineNumbers(false),
        _disableConnectionShortcuts(false),
        _batchSize(50),
        _textFontFamily(""),
        _textFontPointSize(-1),
        _mongoTimeoutSec(10),
        _shellTimeoutSec(15),
        _imported(false)        
    {
        if (!QDir().mkpath(ConfigDir))
            LOG_MSG("ERROR: Could not create settings path: " + ConfigDir, mongo::logger::LogSeverity::Error());

        RoboCrypt::initKey();
        if (!load()) {  // if load fails (probably due to non-existing config. file or directory)
            save();     // create empty settings file
            load();     // try loading again for the purpose of import from previous Robomongo versions
        }

        LOG_MSG("SettingsManager initialized in " + ConfigFilePath, mongo::logger::LogSeverity::Info(), false);
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
        if (!QFile::exists(ConfigFilePath))
            return false;

        QFile f(ConfigFilePath);
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
        QVariantMap const& map = convertToMap();

        QFile f(ConfigFilePath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            LOG_MSG("ERROR: Could not write settings to: " + ConfigFilePath, mongo::logger::LogSeverity::Error());
            return false;
        }

        bool ok;
        QJson::Serializer s;
        s.setIndentMode(QJson::IndentFull);
        s.serialize(map, &f, &ok);

        LOG_MSG("Settings saved to: " + ConfigFilePath, mongo::logger::LogSeverity::Info());

        return ok;
    }

    void SettingsManager::addCacheData(QString const& key, QVariant const& value)
    {
        _cacheData.insert(key, value);
    }

    QVariant SettingsManager::cacheData(QString const& key) const
    {
        return _cacheData.value(key);
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

        _uuidEncoding = (UUIDEncoding)encoding;


        // 3. Load view mode
        if (map.contains("viewMode")) {
            int viewMode = map.value("viewMode").toInt();
            if (viewMode > 2 || viewMode < 0)
                viewMode = Custom; // Default View Mode
            _viewMode = (ViewMode)viewMode;
        }
        else {
            _viewMode = Custom; // Default View Mode
        }

        _autoExpand = map.contains("autoExpand") ? map.value("autoExpand").toBool() : true;
        _autoExec = map.contains("autoExec") ? map.value("autoExec").toBool() : true;
        _minimizeToTray = map.contains("minimizeToTray") ? map.value("minimizeToTray").toBool() : false;
        _lineNumbers = map.contains("lineNumbers") ? map.value("lineNumbers").toBool() : false;
        _imported = map.contains("imported") ? map.value("imported").toBool() : false;
        _programExitedNormally = map.contains("programExitedNormally") ? 
                                 map.value("programExitedNormally").toBool() : true;

        _disableHttpsFeatures = map.contains("disableHttpsFeatures") ? 
                                map.value("disableHttpsFeatures").toBool() : false;

        _debugMode = map.contains("debugMode") ? map.value("debugMode").toBool() : false;

        // 4. Load TimeZone
        int timeZone = map.value("timeZone").toInt();
        if (timeZone > 1 || timeZone < 0)
            timeZone = 0;

        _timeZone = (SupportedTimes)timeZone;
        _loadMongoRcJs = map.value("loadMongoRcJs").toBool();
        _disableConnectionShortcuts = map.value("disableConnectionShortcuts").toBool();
        
        if (map.contains("acceptedEulaVersions")) 
            _acceptedEulaVersions = map.value("acceptedEulaVersions").toStringList().toSet();
        
        if (map.contains("dbVersionsConnected"))
            _dbVersionsConnected = map.value("dbVersionsConnected").toStringList().toSet();
        
        // Load anonymousID
        _anonymousID = getOrCreateAnonymousID(map);

        // Load AutocompletionMode
        if (map.contains("autocompletionMode")) {
            int autocompletionMode = map.value("autocompletionMode").toInt();
            if (autocompletionMode < 0 || autocompletionMode > 2)
                autocompletionMode = AutocompleteAll; // Default Mode
            _autocompletionMode = (AutocompletionMode)autocompletionMode;
        }
        else {
            _autocompletionMode = AutocompleteAll; // Default Mode
        }

        // Load Batch Size
        _batchSize = map.value("batchSize").toInt();
        if (_batchSize == 0)
            _batchSize = 50;

        if (map.contains("checkForUpdates"))
            _checkForUpdates = map.value("checkForUpdates").toBool();

        _currentStyle = map.value("style").toString();
        if (_currentStyle.isEmpty()) {
            _currentStyle = AppStyle::StyleName;
        }

        // Load font information
        _textFontFamily = map.value("textFontFamily").toString();
        _textFontPointSize = map.value("textFontPointSize").toInt();

        if (map.contains("mongoTimeoutSec")) {
            _mongoTimeoutSec = map.value("mongoTimeoutSec").toInt();
        }

        if (map.contains("shellTimeoutSec")) {
            _shellTimeoutSec = map.value("shellTimeoutSec").toInt();
        }

        // 5. Load connections
        _connections.clear();

        QVariantList const& list = map.value("connections").toList();
        for (auto const& conn : list) {
            auto connSettings = new ConnectionSettings(false);
            connSettings->fromVariant(conn.toMap());
            addConnection(connSettings);
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

        _cacheData = map.value("cacheData").toMap();

        // Load connection settings from previous versions of Robomongo
        importFromOldVersion();
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
        
        // 8. Save acceptedEulaVersions array
        QJsonArray arr;
        for (auto const& str : _acceptedEulaVersions)
            arr.push_back(str);

        map.insert("acceptedEulaVersions", arr.toVariantList());

        // x. Save unique set of db versions connected
        QJsonArray dbVersionsArr;
        for (auto const& version : _dbVersionsConnected)
            dbVersionsArr.push_back(version);

        map.insert("dbVersionsConnected", dbVersionsArr.toVariantList());

        // 9. Save batchSize
        map.insert("batchSize", _batchSize);
        map.insert("checkForUpdates", _checkForUpdates);
        map.insert("mongoTimeoutSec", _mongoTimeoutSec);
        map.insert("shellTimeoutSec", _shellTimeoutSec);

        // 10. Save style
        map.insert("style", _currentStyle);

        // 11. Save font information
        map.insert("textFontFamily", _textFontFamily);
        map.insert("textFontPointSize", _textFontPointSize);

        // 12. Save connections
        QVariantList list;
        for (auto const conn : _connections) 
            list.append(conn->toVariant().toMap());

        map.insert("connections", list);
        map.insert("autoExec", _autoExec);
        map.insert("minimizeToTray", _minimizeToTray);
        map.insert("toolbars", _toolbars);
        map.insert("imported", _imported);
        map.insert("anonymousID", _anonymousID);
        map.insert("cacheData", _cacheData);
        map.insert("programExitedNormally", _programExitedNormally);
        map.insert("disableHttpsFeatures", _disableHttpsFeatures);
        map.insert("debugMode", _debugMode);
        
        return map;
    }

    QString SettingsManager::getOrCreateAnonymousID(QVariantMap const& map) const
    {
        QString anonymousID = "";

        // If anonymousID has never been created or is empty, create a new one. Otherwise load the existing.
        if (map.contains("anonymousID")) {
            QUuid id = map.value("anonymousID").toString();
            if (!id.isNull())
                anonymousID = id.toString();
        }

        // Search and import "anonymousID" from other Studio 3T config files
        for (auto const& zipFileAndConfigFile : S_3T_ZipFile_And_ConfigFile_List) {
            if (!anonymousID.isEmpty())
                break;

            QUuid const& id = extractAnonymousIDFromZip(zipFileAndConfigFile.first, zipFileAndConfigFile.second);
            if (!id.isNull())
                anonymousID = id.toString();
        }
                 
        // Search and import "anonymousID" from other Robo 3T old config files starting from latest
        for (auto const& oldConfigFile : _configFilesOfOldVersions) {         
            if (!anonymousID.isEmpty())
                break;

            // Don't import from 1.1-Beta due to a problem where Beta might have redundantly created new UUID 
            if (oldConfigFile == CONFIG_FILE_1_1_0_BETA)
                continue;

            // Stop searching in 1_0_RC1 or older versions, "anonymousID" is introduced in version 1.0
            if (oldConfigFile == CONFIG_FILE_1_0_RC1)
                break;

            anonymousID = extractAnonymousID(oldConfigFile);
        }

        // Search and import "anonymousID" from any other (ideally newer version) Robo 3T config files 
        if (anonymousID.isEmpty()) {
            auto const dir1 = QString("%1/.3T/robo-3t").arg(QDir::homePath());
            QDirIterator iter1 { dir1, QStringList() << "robo*.json", QDir::Files, 
                                 QDirIterator::Subdirectories };
            while (iter1.hasNext()) {
                anonymousID = extractAnonymousID(iter1.next());
                if (!anonymousID.isEmpty())
                    break;
            }

            if (anonymousID.isEmpty()) {
                auto const dir2 = QString("%1/.3T/robomongo").arg(QDir::homePath());
                QDirIterator iter2 { dir2, QStringList() << "robo*.json", QDir::Files, 
                                     QDirIterator::Subdirectories };
                while (iter2.hasNext()) {
                    anonymousID = extractAnonymousID(iter2.next());
                    if (!anonymousID.isEmpty())
                        break;
                }
            }
        }

        // Couldn't find/import any, create a new anonymousID
        if (anonymousID.isEmpty())
            anonymousID = QUuid::createUuid().toString();

        anonymousID.remove('{');
        anonymousID.remove('}');

        return anonymousID;
    }

    /**
     * Adds connection to the end of list and set it's uniqueID
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

    ConnectionSettings* SettingsManager::getConnectionSettingsByUuid(QString const& uuid) const
    {
        for (auto const connSettings : _connections){
            if (connSettings->uuid() == uuid)
                return connSettings;
        }

        LOG_MSG("Failed to find connection settings object by UUID.", mongo::logger::LogSeverity::Warning());
        return nullptr;
    }

    ConnectionSettings* SettingsManager::getConnectionSettingsByUuid(std::string const& uuid) const
    {
        return getConnectionSettingsByUuid(QString::fromStdString(uuid));
    }

    void SettingsManager::setCurrentStyle(const QString& style)
    {
        _currentStyle = style;
    }

    void SettingsManager::setTextFontFamily(const QString& fontFamily)
    {
        _textFontFamily = fontFamily;
    }

    void SettingsManager::setTextFontPointSize(int pointSize) {
        _textFontPointSize = pointSize > 0 ? pointSize : -1;
    }

    void SettingsManager::reorderConnections(const ConnectionSettingsContainerType &connections)
    {
        _connections = connections;
    }

    void SettingsManager::setToolbarSettings(const QString toolbarName, const bool visible)
    {
        _toolbars[toolbarName] = visible;
    }

    void SettingsManager::importFromOldVersion()
    {
        if (_imported)
            return;

        // Import only from the latest version
        for (auto const& configFile : _configFilesOfOldVersions) {
            if (QFile::exists(configFile)) {
                importFromFile(configFile);
                setImported(true);
                return;
            }
        }
    }

    bool SettingsManager::importConnectionsFrom_0_8_5()
    {
        // Load old configuration file (used till version 0.8.5)

        if (!QFile::exists(CONFIG_FILE_0_8_5))
            return false;

        QFile oldConfigFile(CONFIG_FILE_0_8_5);
        if (!oldConfigFile.open(QIODevice::ReadOnly))
            return false;

        bool ok;
        QJson::Parser parser;
        QVariantMap vmap = parser.parse(oldConfigFile.readAll(), &ok).toMap();
        if (!ok)
            return false;

        QVariantList vconns = vmap.value("connections").toList();
        for (QVariantList::iterator itconn = vconns.begin(); itconn != vconns.end(); ++itconn)
        {
            QVariantMap vconn = (*itconn).toMap();

            auto conn = new ConnectionSettings(false);
            conn->setImported(true);
            conn->setConnectionName(QtUtils::toStdString(vconn.value("connectionName").toString()));
            conn->setServerHost(QtUtils::toStdString(vconn.value("serverHost").toString().left(300)));
            conn->setServerPort(vconn.value("serverPort").toInt());
            conn->setDefaultDatabase(QtUtils::toStdString(vconn.value("defaultDatabase").toString()));

            // SSH settings
            if (vconn.contains("sshAuthMethod")) {
                SshSettings *ssh = conn->sshSettings();
                ssh->setHost(QtUtils::toStdString(vconn.value("sshHost").toString()));
                ssh->setUserName(QtUtils::toStdString(vconn.value("sshUserName").toString()));
                ssh->setPort(vconn.value("sshPort").toInt());
                ssh->setUserPassword(QtUtils::toStdString(vconn.value("sshUserPassword").toString()));
                ssh->setPublicKeyFile(QtUtils::toStdString(vconn.value("sshPublicKey").toString()));
                ssh->setPrivateKeyFile(QtUtils::toStdString(vconn.value("sshPrivateKey").toString()));
                ssh->setPassphrase(QtUtils::toStdString(vconn.value("sshPassphrase").toString()));

                int const auth = vconn.value("sshAuthMethod").toInt();
                ssh->setEnabled(auth == 1 || auth == 2);
                ssh->setAuthMethod(auth == 2 ? "publickey" : "password");
            }

            // SSL settings
            if (vconn.contains("sshEnabled")) {
                SslSettings *ssl = conn->sslSettings();
                ssl->enableSSL(vconn.value("enabled").toBool());
                ssl->setPemKeyFile(QtUtils::toStdString(vconn.value("sslPemKeyFile").toString()));
            }

            // Credentials
            QVariantList vcreds = vconn.value("credentials").toList();
            for (QVariantList::const_iterator itcred = vcreds.begin(); itcred != vcreds.end(); ++itcred) {
                QVariantMap vcred = (*itcred).toMap();

                auto cred = new CredentialSettings();
                cred->setUserName(vcred.value("userName").toString().toStdString());
                cred->setUserPassword(vcred.value("userPassword").toString().toStdString());
                cred->setDatabaseName(vcred.value("databaseName").toString().toStdString());
                cred->setMechanism("MONGODB-CR");
                cred->setUseManuallyVisibleDbs(vcred.value("useManuallyVisibleDbs").toBool());
                cred->setManuallyVisibleDbs(vcred.value("manuallyVisibleDbs").toString().toStdString());
                cred->setEnabled(vcred.value("enabled").toBool());

                conn->addCredential(cred);
            }

            // Check that we didn't have similar connection
            bool matched = false;
            for (std::vector<ConnectionSettings*>::const_iterator it = _connections.begin(); it != _connections.end(); ++it) {
                ConnectionSettings *econn = *it;    // Existing connection

                if (conn->serverPort() != econn->serverPort() ||
                    conn->serverHost() != econn->serverHost() ||
                    conn->defaultDatabase() != econn->defaultDatabase())
                    continue;

                CredentialSettings *cred = conn->primaryCredential();
                CredentialSettings *ecred = econn->primaryCredential();
                if (cred->databaseName() != ecred->databaseName() ||
                    cred->userName() != ecred->userName() ||
                    cred->userPassword() != ecred->userPassword() ||
                    cred->enabled() != ecred->enabled())
                    continue;

                SshSettings *ssh = conn->sshSettings();
                SshSettings *essh = econn->sshSettings();
                if (ssh->enabled() != essh->enabled() ||
                    ssh->port() != essh->port() ||
                    ssh->host() != essh->host() ||
                    ssh->privateKeyFile() != essh->privateKeyFile() ||
                    ssh->userPassword() != essh->userPassword() ||
                    ssh->userName() != essh->userName())
                    continue;

                matched = true;
                break;
            }

            // Import connection only if we didn't find similar one
            if (!matched)
                addConnection(conn);
        }

        return true;
    }

    bool SettingsManager::importFromFile(QString const& oldConfigFilePath)
    {
        if (oldConfigFilePath == CONFIG_FILE_0_8_5) {
            importConnectionsFrom_0_8_5();
            return true;
        }

        if (!QFile::exists(oldConfigFilePath))
            return false;

        QFile oldConfigFile(oldConfigFilePath);
        if (!oldConfigFile.open(QIODevice::ReadOnly))
            return false;

        bool ok;
        QJson::Parser parser;
        QVariantMap vmap = parser.parse(oldConfigFile.readAll(), &ok).toMap();
        if (!ok)
            return false;

        //// Import keys
        _autoExpand      = vmap.value("autoExpand").toBool();
        _lineNumbers     = vmap.value("lineNumbers").toBool();
        _debugMode       = vmap.value("debugMode").toBool();
        _shellTimeoutSec = vmap.value("shellTimeoutSec").toInt();
        
        //// Import connections
        for (auto const& vcon : vmap.value("connections").toList()) {
            QVariantMap const& vconn = vcon.toMap();
            auto connSettings = new ConnectionSettings(false);
            connSettings->fromVariant(vconn);
            connSettings->setImported(true);
            addConnection(connSettings);
        }

        return true;
    }

    int SettingsManager::importedConnectionsCount() {
        return (std::count_if(_connections.cbegin(), _connections.cend(), 
            [](auto conn) { return conn->imported(); }
        ));
    }

    QString extractAnonymousIDFromZip(QString const& zipFile, QString const& propfile)
    {
        QZipReader zipReader(zipFile);
        if (!zipReader.exists() || !zipReader.isReadable()) 
            return QString("");       

        QXmlStreamReader reader(zipReader.fileData(propfile));
        while (!reader.atEnd()) {
            reader.readNext();
            if (reader.text().toString() == "AnonymousID") {
                reader.readNext();
                reader.readNext();
                reader.readNext();
                reader.readNext();
                return reader.text().toString();
            }
        }

        return QString("");
    }

    QString extractAnonymousID(QString const& configFilePath)
    {
        if (!QFile::exists(configFilePath))
            return QString("");

        QFile oldConfigFile(configFilePath);
        if (!oldConfigFile.open(QIODevice::ReadOnly))
            return QString("");

        bool ok = false;
        QJson::Parser parser;
        QVariantMap const& map = parser.parse(oldConfigFile.readAll(), &ok).toMap();
        if (!ok)
            return QString("");

        QString anonymousID;
        if (map.contains("anonymousID")) {
            QUuid const& id = map.value("anonymousID").toString();
            if (!id.isNull())
                anonymousID = id.toString();
        }

        anonymousID.remove('{');
        anonymousID.remove('}');

        return anonymousID;
    }
}
