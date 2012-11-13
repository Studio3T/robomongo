#include "SettingsManager.h"

// Qt libs
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QVariantList>

#include "AppRegistry.h"
#include "ConnectionRecord.h"

// Third party libs
#include "boost/ptr_container/ptr_vector.hpp"
#include "qjson/parser.h"
#include "qjson/serializer.h"

using namespace Robomongo;

/**
 * Creates SettingsManager for config file in default location
 * ~/.config/robomongo/robomongo.json
 */
SettingsManager::SettingsManager()
{
    _version = "1.0";
    _configPath = QString("%1/.config/robomongo/robomongo.json").arg(QDir::homePath());
    _configDir  = QString("%1/.config/robomongo").arg(QDir::homePath());

    load();

    qDebug() << "SettingsManager initialized in " << _configPath;
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
    //s.setIndentMode(QJson::IndentFull);
    s.serialize(map, &f, &ok);

    qDebug() << "Settings saved to: " << _configPath;

    return ok;
}

void SettingsManager::addConnection(ConnectionRecord *connection)
{
    _connections.push_back(connection);
}

void SettingsManager::removeConnection(int index)
{
    _connections.erase(_connections.begin() + index);
}

void SettingsManager::loadFromMap(QVariantMap &map)
{
    // 1. Load connections
    QVariantList list = map.value("connections").toList();
    foreach(QVariant var, list) {
        ConnectionRecord * record = new ConnectionRecord();
        record->fromVariant(var.toMap());
        addConnection(record);
    }

    // 2. Load version
    _version = map.value("version").toString();
}

QVariantMap SettingsManager::convertToMap() const
{
    QVariantMap map;

    // 1. Save version
    map.insert("version", _version);

    // 2. Save connections
    QVariantList list;
    for (size_t i = 0; i < _connections.size(); i++)
    {
        const ConnectionRecord & record = _connections.at(i);
        QVariantMap rm = record.toVariant().toMap();
        list.append(rm);
    }

    map.insert("connections", list);
    return map;
}
