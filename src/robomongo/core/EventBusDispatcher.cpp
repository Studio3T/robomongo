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
        const QList<QObject*> &recivers = wrapper->receivers();
        for (QList<QObject*>::const_iterator it = recivers.begin(); it != recivers.end(); ++it) {
            QMetaObject::invokeMethod(*it, "handle", QGenericArgument(typeName, &event));
        }

        return true;
    }
}
