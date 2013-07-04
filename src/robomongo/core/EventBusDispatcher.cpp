#include "robomongo/core/EventBusDispatcher.h"
#include "robomongo/core/EventWrapper.h"

namespace Robomongo
{

    EventBusDispatcher::EventBusDispatcher(QObject *parent) :
        QObject(parent)
    {

    }
    bool EventBusDispatcher::event(QEvent *qevent)
    {
        EventWrapper *wrapper = dynamic_cast<EventWrapper *>(qevent);

        if (!wrapper)
            return false;

        Event *event = wrapper->event();

        const char *typeName = event->typeString();

        foreach(QObject *receiver, wrapper->receivers()) {
            QMetaObject::invokeMethod(receiver, "handle", QGenericArgument(typeName, &event));
        }

        return true;
    }
}
