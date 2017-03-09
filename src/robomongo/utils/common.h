#pragma once

#include <string>
#include <QObject>

namespace Robomongo
{
    class Event;
    class EventBus;

    // Special handler designed to be used in MongoDatanase and MongoServer classes and only for 
    // event->isError() is true case.
    void genericEventErrorHandler(Event *event, const std::string &userFriendlyMessage, EventBus* bus, 
                                  QObject* sender);

    bool fileExists(const QString& filePath);
}
