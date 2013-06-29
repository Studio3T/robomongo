#pragma once
class QObject;
namespace Robomongo
{
    class EventBusDispatcher;
    struct EventBusSubscriber
    {
        EventBusSubscriber(EventBusDispatcher *dispatcher, QObject *receiver, QObject *sender = 0);
        EventBusDispatcher *dispatcher;
        QObject *receiver;
        QObject *sender;
    };
}
