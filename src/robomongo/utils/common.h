#pragma once

#include <string>
#include <QObject>

namespace Robomongo
{
    class Event;
    class EventBus;
    //class QObject;

    void genericResponseHandler(Event *event, const std::string &userFriendlyMessage, EventBus* bus, QObject* sender);

}
