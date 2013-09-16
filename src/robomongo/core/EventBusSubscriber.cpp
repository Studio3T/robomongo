#include "robomongo/core/EventBusSubscriber.h"
#include "robomongo/core/EventBusDispatcher.h"

namespace Robomongo
{
    EventBusSubscriber::EventBusSubscriber(EventBusDispatcher *dispatcher, QObject *receiver, QObject *sender) :
        receiver(receiver),
        dispatcher(dispatcher),
        sender(sender) {}
}
