#include "robomongo/core/EventWrapper.h"
namespace Robomongo
{
    EventWrapper::EventWrapper(Event *event, QList<QObject *> receivers) 
        : QEvent(event->type()), _event(event), _receivers(receivers) {}

    EventWrapper::EventWrapper(Event *event, QObject * receiver)
        : QEvent(event->type()), _event(event), _receivers(QList<QObject *>() << receiver ) {}

    Event *EventWrapper::event() const 
    {
        return _event.get(); 
    }

    const QList<QObject *> &EventWrapper::receivers() const 
    {
        return _receivers;
    }
}
