#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <QEvent>
#include <QMultiHash>
#include "EventBusDispatcher.h"
#include <QMutex>

namespace Robomongo
{
    class Event;
    class EventBusSubscriber;

    class EventBus : QObject
    {
        Q_OBJECT

    public:
        EventBus();
        ~EventBus();

        /**
         * @brief Publishes event
         * @param sender - object, that emits this event
         */
        void publish(Event *event);

        void send(QObject *receiver, Event *event);
        void send(QList<QObject *> receivers, Event *event);

        /**
         * @brief Subscribe to specified event
         */
        void subscribe(QObject *receiver, QEvent::Type type, QObject *sender = NULL);

    protected:
        EventBusDispatcher *dispatcher(QThread *thread);

    public slots:

        void unsubscibe(QObject *receiver);

    private:
        QMutex _lock;
        QMultiHash<QEvent::Type, EventBusSubscriber *> _subscribersByEventType;
        QHash<QThread *, EventBusDispatcher *> _dispatchersByThread;
    };
}

#endif // DISPATCHER_H
