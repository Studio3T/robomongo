#include "robomongo/core/settings/SettingsManager.h"

#include <QDir>
#include <QDebug>
#include <QFile>
#include <QVariantList>
#include <qjson/parser.h>
#include <qjson/serializer.h>

#include "robomongo/core/settings/ConnectionSettings.h"

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
    SettingsManager::SettingsManager(QObject *parent) 
        : QObject(parent),_version(SchemaVersion),_uuidEncoding(DefaultEncoding),_timeZone(Utc),_viewMode(Robomongo::Tree)
    {
        load();
        qDebug() << "SettingsManager initialized in " << _configPath;
    }

    SettingsManager::~SettingsManager()
    {
        qDeleteAll(_connections);
    }

    /**
     * Load settings from config file.
     * @return true if success, false otherwise
     */
    bool SettingsManager::load()
    {
        bool result = false;
        if(QFile::exists(_configPath))
        {
            QFile f(_configPath);
            if(f.open(QIODevice::ReadOnly))
            {
                QJson::Parser parser;
                bool ok;
                QVariantMap map = parser.parse(f.readAll(), &ok).toMap();
                if(ok)
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

                    // 4. Load TimeZone
                    int timeZone = map.value("timeZone").toInt();
                    if (timeZone > 2 || timeZone < 0)
                        timeZone = 0;

                    _timeZone = (SupportedTimes) timeZone;

                    // 5. Load connections
                    _connections.clear();

                    QVariantList list = map.value("connections").toList();
                    for(QVariantList::iterator it = list.begin();it!=list.end();++it) {
                        ConnectionSettings *record = new ConnectionSettings((*it).toMap());
                        _connections.append(record);
                    }
                    result = true;
                }
            }
        }
        return result;
    }

    /**
     * Saves all settings to config file.
     * @return true if success, false otherwise
     */
    bool SettingsManager::save()
    {
        bool result = false;
        QVariantMap map;

        // 1. Save schema version
        map.insert("version", SchemaVersion);

        // 2. Save UUID encoding
        map.insert("uuidEncoding", _uuidEncoding);

        // 3. Save TimeZone encoding
        map.insert("timeZone", _timeZone);

        // 4. Save view mode
        map.insert("viewMode", _viewMode);

        // 5. Save connections
        QVariantList list;

        for(QList<ConnectionSettings *>::const_iterator it = _connections.begin();it!=_connections.end();++it) {
            QVariantMap rm = (*it)->toVariant().toMap();
            list.append(rm);
        }

        map.insert("connections", list);
        if (QDir().mkpath(_configDir))
        {
            QFile f(_configPath);
            if(f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                QJson::Serializer s;
                s.setIndentMode(QJson::IndentFull);
                s.serialize(map, &f, &result);
                qDebug() << "Settings saved to: " << _configPath;
            }
        }
        return result;
    }

    /**
     * Adds connection to the end of list
     */
    void SettingsManager::addConnection(ConnectionSettings *connection)
    {
        _connections.append(connection);
        emit connectionAdded(connection);
    }

    /**
     * Update connection
     */
    void SettingsManager::updateConnection(ConnectionSettings *connection)
    {
        if (_connections.contains(connection))
            emit connectionUpdated(connection);
    }

    /**
     * Removes connection by index
     */
    void SettingsManager::removeConnection(ConnectionSettings *connection)
    {
        if(_connections.removeOne(connection)){
            emit connectionRemoved(connection);
            delete connection;
        }
    }

    void SettingsManager::reorderConnections(const QList<ConnectionSettings *> &connections)
    {
        _connections = connections;
    }

}
