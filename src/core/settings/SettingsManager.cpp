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

SettingsManager::SettingsManager()
{
    _version = "1.0";
    _configPath = QString("%1/.config/robomongo/robomongo.json").arg(QDir::homePath());
    _configDir  = QString("%1/.config/robomongo").arg(QDir::homePath());

    load();

    qDebug() << "SettingsManager initialized in " << _configPath;
}

SettingsManager::~SettingsManager()
{
    qDebug() << "SettingsManager released";
}

bool SettingsManager::load()
{
    if (!QFile::exists(_configPath))
        return false;

    QFile f(_configPath);
    if(!f.open(QIODevice::ReadOnly))
        return false;

    QJson::Parser parser;
    bool ok;

    QVariantMap data = parser.parse(f.readAll(), &ok).toMap();
    if( !ok )
        return false;

    // 1. Load connections
    QVariantList list = data.value("connections").toList();
    foreach(QVariant map, list) {
        ConnectionRecord * record = new ConnectionRecord();
        record->fromVariant(map.toMap());
        addConnection(record);
    }

    // 2. Load version
    _version = data.value("version").toString();

    return true;
}

bool SettingsManager::save()
{
    QVariantMap map;

    // 1. Save version
    map.insert("version", _version);

    // 2. Save connections
    QVariantList list;
    for (int i = 0; i < _connections.size(); i++)
    {
        const ConnectionRecord & record = _connections.at(i);
        QVariantMap rm = record.toVariant().toMap();
        list.append(rm);
    }

    map.insert("connections", list);

    bool ok;
    {
        if (!QDir().mkpath(_configDir))
            return false;

        QFile f(_configPath);
        if(!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;

        QJson::Serializer s;
        //s.setIndentMode(QJson::IndentFull);
        s.serialize(map, &f, &ok);
    }

    qDebug() << "SettingsManager saved file: " << _configPath;

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
