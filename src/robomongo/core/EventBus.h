#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <QEvent>
#include <QMultiHash>
#include <QMutex>

#include "robomongo/core/EventBusDispatcher.h"

namespace Robomongo
{
    class Event;
    class EventWrapper;
    class EventBusSubscriber;

    /**
     * @brief The EventBus class
     * @threadsafe
     */
    class EventBus : public QObject
    {
        Q_OBJECT

    public:
        /**
         * @brief Creates EventBus.
         */
        EventBus();

        /**
         * @brief Cleanups EventBus.
         */
        ~EventBus();

        /**
         * @brief Publishes specified event.
         * @param sender - object, that emits this event
         */
        void publish(Event *event);

        /**
         * @brief Sends 'event' to 'receiver'.
         */
        void send(QObject *receiver, Event *event);

        /**
         * @brief Sends 'event' to list of 'receivers'.
         */
        void send(QList<QObject *> receivers, Event *event);

        /**
         * @brief Subscribes 'receiver' to event of specified 'type'.
         * Optionally you can specify exact send
         */
        void subscribe(QObject *receiver, QEvent::Type type, QObject *sender = NULL);

    public slots:

        void unsubscibe(QObject *receiver);

    private:

        /**
         * @brief Returns dispatcher for specified thread. If there is no dispatcher
         * for this thread registered, new dispatcher will be created and moved
         * to 'thread' thread.
         */
        EventBusDispatcher *dispatcher(QThread *thread);

        void sendEvent(EventBusDispatcher *dispatcher, EventWrapper *wrapper);

    private:

        QMutex _lock;
        QMultiHash<QEvent::Type, EventBusSubscriber *> _subscribersByEventType;
        QHash<QThread *, EventBusDispatcher *> _dispatchersByThread;
    };
}

#endif // DISPATCHER_H
