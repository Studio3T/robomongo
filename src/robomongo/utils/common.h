#pragma once

// todo: rename to utils.h

#include <string>
#include <QObject>

namespace Robomongo /* todo ::utils */
{
    class Event;
    class EventBus;

    // Special handler designed to be used in MongoDatanase and MongoServer classes and only for 
    // event->isError() is true case.
    void genericEventErrorHandler(
        Event *event, const std::string &userFriendlyMessage, EventBus* bus, QObject* sender);

    bool fileExists(const QString& filePath);

    template<typename T>
    bool vectorContains(std::vector<T> const& vec, T const& value);

    QVariant getSetting(QString const& key);
    void saveSetting(QString const& key, QVariant const& value);
}