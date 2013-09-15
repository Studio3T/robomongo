#pragma once
#include <boost/scoped_ptr.hpp>
#include "robomongo/core/Event.h"

namespace Robomongo
{
    class EventWrapper : public QEvent
    {
    public:
        EventWrapper(Event *event, QList<QObject *> receivers);
        EventWrapper(Event *event, QObject * receiver);
        Event *event() const;
        const QList<QObject *> &receivers() const;

    private:
        const boost::scoped_ptr<Event> _event;
        const QList<QObject *> _receivers;
    };
}
