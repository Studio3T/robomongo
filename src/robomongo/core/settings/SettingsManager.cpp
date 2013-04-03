#include "robomongo/core/settings/SettingsManager.h"

#include <QDir>
#include <QDebug>
#include <QFile>
#include <QVariantList>
#include <qjson/parser.h>
#include <qjson/serializer.h>

#include "robomongo/core/settings/ConnectionSettings.h"

using namespace Robomongo;

const QString SettingsManager::SchemaVersion = "1.0";

/**
 * Creates SettingsManager for config file in default location
 * ~/.config/robomongo/robomongo.json
 */
SettingsManager::SettingsManager(QObject *parent) : QObject(parent)
{
    _version = "1.0";
    _configPath = QString("%1/.config/robomongo/robomongo.json").arg(QDir::homePath());
    _configDir  = QString("%1/.config/robomongo").arg(QDir::homePath());
    _uuidEncoding = DefaultEncoding;

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
    if (!QFile::exists(_configPath))
        return false;

    QFile f(_configPath);
    if(!f.open(QIODevice::ReadOnly))
        return false;

    QJson::Parser parser;
    bool ok;

    QVariantMap map = parser.parse(f.readAll(), &ok).toMap();
    if( !ok )
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
    if(!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    bool ok;
    QJson::Serializer s;
    s.setIndentMode(QJson::IndentFull);
    s.serialize(map, &f, &ok);

    qDebug() << "Settings saved to: " << _configPath;

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

    // 3. Load connections
    _connections.clear();

    QVariantList list = map.value("connections").toList();
    foreach(QVariant var, list) {
        ConnectionSettings *record = new ConnectionSettings();
        record->fromVariant(var.toMap());
        _connections.append(record);
    }
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

    // 3. Save connections
    QVariantList list;

    foreach(ConnectionSettings *record, _connections) {
        QVariantMap rm = record->toVariant().toMap();
        list.append(rm);
    }

    map.insert("connections", list);

    return map;
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
    _connections.removeOne(connection);
    emit connectionRemoved(connection);
    delete connection;
}

void SettingsManager::reorderConnections(const QList<ConnectionSettings *> &connections)
{
    _connections = connections;
}

