#ifndef EVENTBUSSUBSCRIBER_H
#define EVENTBUSSUBSCRIBER_H

#include <QObject>
#include "EventBusDispatcher.h"

namespace Robomongo
{
    class EventBusSubscriber
    {
    public:
        explicit EventBusSubscriber(EventBusDispatcher *dispatcher, QObject *receiver, QObject *sender = NULL) :
            receiver(receiver),
            dispatcher(dispatcher),
            sender(sender) {}

        EventBusDispatcher *dispatcher;
        QObject *receiver;
        QObject *sender;
    };
}

#endif // EVENTBUSSUBSCRIBER_H
