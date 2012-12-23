#include "SettingsManager.h"

// Qt libs
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QVariantList>

#include "ConnectionRecord.h"

// Third party libs
#include "qjson/parser.h"
#include "qjson/serializer.h"

using namespace Robomongo;

/**
 * Creates SettingsManager for config file in default location
 * ~/.config/robomongo/robomongo.json
 */
SettingsManager::SettingsManager(QObject *parent) : QObject(parent)
{
    _version = "1.0";
    _configPath = QString("%1/.config/robomongo/robomongo.json").arg(QDir::homePath());
    _configDir  = QString("%1/.config/robomongo").arg(QDir::homePath());

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
    // 1. Load connections
    _connections.clear();

    QVariantList list = map.value("connections").toList();
    foreach(QVariant var, list) {
        ConnectionRecord *record = new ConnectionRecord();
        record->fromVariant(var.toMap());
        _connections.append(record);
    }

    // 2. Load version
    _version = map.value("version").toString();
}

/**
 * Save all settings to map.
 */
QVariantMap SettingsManager::convertToMap() const
{
    QVariantMap map;

    // 1. Save version
    map.insert("version", _version);

    // 2. Save connections
    QVariantList list;

    foreach(ConnectionRecord *record, _connections) {
        QVariantMap rm = record->toVariant().toMap();
        list.append(rm);
    }

    map.insert("connections", list);
    return map;
}

/**
 * Adds connection to the end of list
 */
void SettingsManager::addConnection(ConnectionRecord *connection)
{
    _connections.append(connection);
    emit connectionAdded(connection);
}

/**
 * Update connection
 */
void SettingsManager::updateConnection(ConnectionRecord *connection)
{
    emit connectionUpdated(connection);
}

/**
 * Removes connection by index
 */
void SettingsManager::removeConnection(ConnectionRecord *connection)
{
    _connections.removeOne(connection);
    emit connectionRemoved(connection);
    delete connection;
}

void SettingsManager::reorderConnections(const QList<ConnectionRecord *> &connections)
{
    _connections = connections;
}

