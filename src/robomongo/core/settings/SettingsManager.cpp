#include "robomongo/core/settings/SettingsManager.h"

#include <QDir>
#include <QFile>
#include <QVariantList>
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

namespace
{
        /**
         * @brief Version of schema
         */
        const QString SchemaVersion = "2.0";

         /**
         * @brief Config file absolute path
         *        (usually: /home/user/.config/robomongo/robomongo.json)
         */
        const QString _configPath = QString("%1/.config/robomongo/0.9/robomongo.json").arg(QDir::homePath());

        /**
         * @brief Config file containing directory path
         *        (usually: /home/user/.config/robomongo)
         */
        const QString _configDir = QString("%1/.config/robomongo/0.9").arg(QDir::homePath());
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
        _disableConnectionShortcuts(false),
        _textFontFamily(""),
        _textFontPointSize(-1),
        _lineNumbers(false),
        _loadMongoRcJs(false),
        _imported(false),
        _mongoTimeoutSec(10),
        _shellTimeoutSec(15)
    {
        load();
        LOG_MSG("SettingsManager initialized in " + _configPath, mongo::logger::LogSeverity::Info(), false);
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

        if (!QDir().mkpath(_configDir)) {
            LOG_MSG("ERROR: Could not create settings path: " + _configDir, mongo::logger::LogSeverity::Error());
            return false;
        }

        QFile f(_configPath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            LOG_MSG("ERROR: Could not write settings to: " + _configPath, mongo::logger::LogSeverity::Error());
            return false;
        }

        bool ok;
        QJson::Serializer s;
        s.setIndentMode(QJson::IndentFull);
        s.serialize(map, &f, &ok);

        LOG_MSG("Settings saved to: " + _configPath, mongo::logger::LogSeverity::Info());

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

        _minimizeTray = map.contains("minimizeTray") ?
            map.value("minimizeTray").toBool() : false;

        _lineNumbers = map.contains("lineNumbers") ?
            map.value("lineNumbers").toBool() : false;

        _imported = map.contains("imported") ?
            map.value("imported").toBool() : false;

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

        QVariantList list = map.value("connections").toList();
        for (QVariantList::iterator it = list.begin(); it != list.end(); ++it) {
            ConnectionSettings *record = new ConnectionSettings();
            record->fromVariant((*it).toMap());
            addConnection(record);
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

        // Load connection settings from previous
        // versions of Robomongo
        importPreviousConnections();
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
        map.insert("mongoTimeoutSec", _mongoTimeoutSec);
        map.insert("shellTimeoutSec", _shellTimeoutSec);

        // 9. Save style
        map.insert("style", _currentStyle);

        // 10. Save font information
        map.insert("textFontFamily", _textFontFamily);
        map.insert("textFontPointSize", _textFontPointSize);

        // 11. Save connections
        QVariantList list;

        for (ConnectionSettingsContainerType::const_iterator it = _connections.begin(); it != _connections.end(); ++it) {
            QVariantMap rm = (*it)->toVariant().toMap();
            list.append(rm);
        }

        map.insert("connections", list);

        map.insert("autoExec", _autoExec);

        map.insert("minimizeTray", _minimizeTray);

        map.insert("toolbars", _toolbars);

        map.insert("imported", _imported);

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

    void SettingsManager::importPreviousConnections() {
        // Skip when settings already imported
        if (_imported)
            return;

        // Load old configuration file (used till version 0.8.5)
        const QString oldConfigPath = QString("%1/.config/robomongo/robomongo.json").arg(QDir::homePath());

        if (!QFile::exists(oldConfigPath))
            return;

        QFile f(oldConfigPath);
        if (!f.open(QIODevice::ReadOnly))
            return;

        bool ok;
        QJson::Parser parser;
        QVariantMap vmap = parser.parse(f.readAll(), &ok).toMap();
        if (!ok)
            return;

        QVariantList vconns = vmap.value("connections").toList();
        for (QVariantList::iterator itconn = vconns.begin(); itconn != vconns.end(); ++itconn) {
            QVariantMap vconn = (*itconn).toMap();

            ConnectionSettings *conn = new ConnectionSettings();
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

                CredentialSettings *cred = new CredentialSettings();
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

        // Mark as imported
        setImported(true);
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
}
