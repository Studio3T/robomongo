#pragma once

#include <QObject>

namespace Robomongo
{
    class EventBusDispatcher;
    struct EventBusSubscriber
    {
        EventBusSubscriber(EventBusDispatcher *dispatcher, QObject *receiver, QObject *sender = 0);
        EventBusDispatcher *const dispatcher;
        QObject *const receiver;
        QObject *const sender;
    };
}
