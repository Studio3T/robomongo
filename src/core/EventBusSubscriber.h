#ifndef EVENTBUSSUBSCRIBER_H
#define EVENTBUSSUBSCRIBER_H

#include <QObject>

namespace Robomongo
{
    class EventBusSubscriber
    {
    public:
        explicit EventBusSubscriber(QObject *receiver, QObject *sender = NULL) :
            receiver(receiver),
            sender(sender) {}

        QObject *receiver;
        QObject *sender;
    };
}

#endif // EVENTBUSSUBSCRIBER_H
