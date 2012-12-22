#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <QEvent>
#include <QMultiHash>

namespace Robomongo
{
    class Event;
    class EventBusSubscriber;

    class EventBus : QObject
    {
        Q_OBJECT

    public:
        EventBus();

        /**
         * @brief Publishes event
         * @param sender - object, that emits this event
         */
        void publish(Event *event);

        /**
         * @brief Subscribe to specified event
         */
        void subscribe(QObject *receiver, QEvent::Type type, QObject *sender = NULL);

    public slots:

        void unsubscibe(QObject *receiver);

    private:

        QMultiHash<QEvent::Type, EventBusSubscriber *> _subscribersByEventType;
    };
}

#endif // DISPATCHER_H
