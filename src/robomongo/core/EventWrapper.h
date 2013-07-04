#pragma once
#include <boost/scoped_ptr.hpp>
#include "robomongo/core/Event.h"

namespace Robomongo
{
    class EventWrapper : public QEvent
    {
    public:
        EventWrapper(Event *event, QList<QObject *> receivers);
        Event *event() const;
        QList<QObject *> receivers() const;
    private:
        boost::scoped_ptr<Event> _event;
        QList<QObject *> _receivers;
    };
}
