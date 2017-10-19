#include "robomongo/core/settings/SettingsManager.h"

#include <QDir>
#include <QFile>
#include <QVariantList>
#include <QUuid>
#include <QJsonArray>
#include <QXmlStreamReader>

// Todo: 
// Note: 
//         Currently, this include statement should be maintained manually.
//         The Qt version must be updated if Qt is upgraded. Otherwise it might cause build error. 
//         The correct path will be: PROJECT_QT_VERSION/QtGui/private/qzipreader_p.h
// Details:         
//         Ideally, private class inside qzipreader_p.h should not be used since it is not 
//         a publicly/officially supported API by Qt. It is used here since we are doing a 
//         a very lightweight zip operation which is unzipping a zip archive with one, small file.
//         The alternative is including QuaZip and Zlib third party libraries into CMAKE project and 
//         building, and using them which might be time consuming.
#include <5.7.0/QtGui/private/qzipreader_p.h>

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

namespace Robomongo
{
    // 3T config files
    auto const Studio3T_PropertiesDat = QString("%1/.3T/studio-3t/properties.dat").arg(QDir::homePath());
    auto const DataMongodb_PropertiesDat = QString("%1/.3T/data-man-mongodb/properties.dat").arg(QDir::homePath());
    auto const MongoChefPro_PropertiesDat = QString("%1/.3T/mongochef-pro/properties.dat").arg(QDir::homePath());
    auto const MongoChefEnt_PropertiesDat = QString("%1/.3T/mongochef-enterprise/properties.dat").arg(QDir::homePath());

    const std::vector<std::pair<QString, QString>> S_3T_ZipFile_And_ConfigFile_List
    {
        { Studio3T_PropertiesDat, "Studio3T.properties" },
        { DataMongodb_PropertiesDat, "3T.data-man-mongodb.properties" },
        { MongoChefPro_PropertiesDat, "3T.mongochef-pro.properties" },
        { MongoChefEnt_PropertiesDat, "3T.mongochef-enterprise.properties" }
    };

    // Extract zipFile and find the value of "anonymousID" field in propFile
    QString extractAnonymousIDFromZip(QString const& zipFile, QString const& propfile);

    // Extract "anonymousID" from old config file
    QString extractAnonymousID(QString const& oldConfigFile);

    /**
        * @brief Version of schema
    */
    const QString SchemaVersion = "2.0";

    /**
    * @brief Robomongo config. files of old versions
    */
    const auto CONFIG_FILE_0_8_5 = QString("%1/.config/robomongo/robomongo.json").arg(QDir::homePath());
    const auto CONFIG_FILE_0_9 = QString("%1/.config/robomongo/0.9/robomongo.json").arg(QDir::homePath());
    const auto CONFIG_FILE_1_0_RC1 = QString("%1/.config/robomongo/1.0/robomongo.json").arg(QDir::homePath());
    // Change of path due to re-branding to 3T
    const auto CONFIG_FILE_1_0_0 = QString("%1/.3T/robomongo/1.0.0/robomongo.json").arg(QDir::homePath());
    const auto CONFIG_FILE_1_1_0_BETA = QString("%1/.3T/robomongo/1.1.0-Beta/robomongo.json").arg(QDir::homePath());

    // Important Note: In order to import connections from the latest version found, config. file path of an 
    //                 old version must be defined (i.e. CONFIG_FILE_1_0_0 seen above) and placed into the 
    //                 vector initializer list below in order.
    std::vector<QString> const SettingsManager::_configFilesOfOldVersions
    {
        CONFIG_FILE_1_1_0_BETA,
        CONFIG_FILE_1_0_0,
        CONFIG_FILE_1_0_RC1,
        CONFIG_FILE_0_9,
        CONFIG_FILE_0_8_5,
    };

    std::vector<ConnectionSettings*>  SettingsManager::_connections;
    
    // Temporarily disabling Recent Connections feature
    // std::vector<RecentConnection> SettingsManager::_recentConnections;

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
        QVariantMap map = convertToMap();

        if (!QDir().mkpath(ConfigDir)) {
            LOG_MSG("ERROR: Could not create settings path: " + ConfigDir, mongo::logger::LogSeverity::Error());
            return false;
        }

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

        // 4. Load TimeZone
        int timeZone = map.value("timeZone").toInt();
        if (timeZone > 1 || timeZone < 0)
            timeZone = 0;

        _timeZone = (SupportedTimes)timeZone;
        _loadMongoRcJs = map.value("loadMongoRcJs").toBool();
        _disableConnectionShortcuts = map.value("disableConnectionShortcuts").toBool();
        
        if (map.contains("acceptedEulaVersions")) 
            _acceptedEulaVersions = map.value("acceptedEulaVersions").toStringList().toSet();
        
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

        /* Temporarily disabling Recent Connections feature
        // Load recent connections
        _recentConnections.clear();
        QVariantList const& rlist = map.value("recentConnections").toList();
        for (auto const& rconn : rlist) {
            _recentConnections.push_back(RecentConnection(rconn.toMap().value("uuid").toString(),
                                                          rconn.toMap().value("name").toString().toStdString()));
        }
        */

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
        importConnections();
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

        /* Temporarily disabling Recent Connections feature
        // 13. Save recent connections
        QVariantList recentConnsList;
        QVariantMap recentConnMap;
        for (auto const& rconn : _recentConnections) {
            recentConnMap.insert("uuid", rconn.uuid);
            recentConnMap.insert("name", QString::fromStdString(rconn.name));
            recentConnsList.append(recentConnMap);
        }
        map.insert("recentConnections", recentConnsList);
        */

        map.insert("autoExec", _autoExec);
        map.insert("minimizeToTray", _minimizeToTray);
        map.insert("toolbars", _toolbars);
        map.insert("imported", _imported);
        map.insert("anonymousID", _anonymousID);
        map.insert("cacheData", _cacheData);

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

/* Temporarily disabling Recent Connections feature
    void SettingsManager::addRecentConnection(ConnectionSettings *connection)
    {
        _recentConnections.push_back(RecentConnection(connection->uuid(), connection->connectionName()));
    }

    void SettingsManager::deleteRecentConnection(ConnectionSettings *conn)
    {
        for (int i = 0; i < _recentConnections.size(); ++i) {
            if (_recentConnections[i].uuid == conn->uuid())
                _recentConnections.erase(_recentConnections.begin() + i);
        }
    }

    void SettingsManager::setRecentConnections(std::vector<ConnectionSettings const*> const& recentConns)
    {
        _recentConnections.clear();

        for(auto rconn : recentConns)
            _recentConnections.push_back(RecentConnection(rconn->uuid(), rconn->connectionName()));
    }

    void SettingsManager::clearRecentConnections()
    {
        _recentConnections.clear();
    }
*/

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

    void SettingsManager::importConnections()
    {
        // Do not import if already imported
        if (_imported)
            return;

        // Find and import 'only' from the latest version to current version
        for (auto const& configFile : _configFilesOfOldVersions) {
            if (QFile::exists(configFile)) {
                importConnectionsFromOldVersion(configFile);
                setImported(true);  // Mark as imported
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

                int auth = vconn.value("sshAuthMethod").toInt();
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
                cred->setUserName(QtUtils::toStdString(vcred.value("userName").toString()));
                cred->setUserPassword(QtUtils::toStdString(vcred.value("userPassword").toString()));
                cred->setDatabaseName(QtUtils::toStdString(vcred.value("databaseName").toString()));
                cred->setMechanism("MONGODB-CR");
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

    bool SettingsManager::importConnectionsFromOldVersion(QString const& oldConfigFilePath)
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

        QVariantList const& vconns = vmap.value("connections").toList();
        for (auto const& vcon : vconns)
        {
            QVariantMap const& vconn = vcon.toMap();
            auto connSettings = new ConnectionSettings(false);
            connSettings->fromVariant(vconn);
            connSettings->setImported(true);

            addConnection(connSettings);
        }

        return true;
    }

    int SettingsManager::importedConnectionsCount() {
        int count = 0;
        for (std::vector<ConnectionSettings*>::const_iterator it = _connections.begin(); it != _connections.end(); ++it) {
            ConnectionSettings *conn = *it;
            if (conn->imported())
                ++count;
        }

        return count;
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

    QString extractAnonymousID(QString const& oldConfigFilePath)
    {
        if (!QFile::exists(oldConfigFilePath))
            return QString("");

        QFile oldConfigFile(oldConfigFilePath);
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
