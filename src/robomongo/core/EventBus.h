#pragma once
#include <QEvent>
#include <vector>
#include <QMutex>

#include "robomongo/core/EventBusDispatcher.h"

namespace Robomongo
{
    class Event;
    class EventWrapper;
    struct EventBusSubscriber;

    /**
     * @brief The EventBus class
     * @threadsafe
     */
    class EventBus : public QObject
    {
        Q_OBJECT

    public:
        typedef std::pair<QEvent::Type, EventBusSubscriber *> subscribersType;
        typedef std::vector<subscribersType > subscribersContainerType;
        typedef std::pair<QThread *, EventBusDispatcher *> dispatchersType;
        typedef std::vector<dispatchersType> dispatchersContainerType;
        EventBus();
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

    public Q_SLOTS:
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
        subscribersContainerType _subscribersByEventType;
        dispatchersContainerType _dispatchersByThread;
    };
}
