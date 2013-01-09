#pragma once

#include <boost/scoped_ptr.hpp>

#include "robomongo/core/Event.h"

namespace Robomongo
{
    class EventWrapper : public QEvent
    {
    public:
        EventWrapper(Event *event, QList<QObject *> receivers) :
            QEvent(event->type()),
            _event(event),
            _receivers(receivers) {}

        Event *event() const { return _event.get(); }
        QList<QObject *> receivers() const { return _receivers; }

    private:
        boost::scoped_ptr<Event> _event;
        QList<QObject *> _receivers;
    };
}
