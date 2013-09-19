#include "robomongo/core/EventBus.h"

#include <QObject>
#include <QCoreApplication>
#include <QThread>
#include <QMutexLocker>

#include "robomongo/core/EventBusDispatcher.h"
#include "robomongo/core/EventBusSubscriber.h"
#include "robomongo/core/Event.h"
#include "robomongo/core/EventWrapper.h"
#include "robomongo/core/utils/QtUtils.h"

namespace
{
    struct RemoveIfReciver : public std::unary_function<const Robomongo::EventBus::SubscribersType&, bool>
    {
        RemoveIfReciver(QObject *receiver) : _receiver(receiver) {}

        bool operator()(const Robomongo::EventBus::SubscribersType& item) const {
            if (item.second->receiver == _receiver) {
                delete item.second;
                return true;
            }
            return false;
        }

        QObject *_receiver;
    };

    struct FindIfReciver : public std::unary_function<const Robomongo::EventBus::SubscribersType&, bool>
    {
        FindIfReciver(QThread *thread) : _thread(thread) {}
        bool operator()(const Robomongo::EventBus::DispatchersType &item) const {
            if (item.first == _thread) {
                return true;
            }
            return false;
        }
        QThread *_thread;
    };
}

namespace Robomongo
{
    EventBus::EventBus() : QObject(),
        _lock(QMutex::Recursive)
    {
    }

    EventBus::~EventBus()
    {
        for (DispatchersContainerType::iterator it = _dispatchersByThread.begin(); it != _dispatchersByThread.end(); ++it) {
            DispatchersType item = *it;
            delete item.second;
        }
    }

    void EventBus::publish(Event *event)
    {
        QMutexLocker lock(&_lock);
        QList<QObject*> theReceivers;
        EventBusDispatcher *dis = NULL;
        for (SubscribersContainerType::iterator it = _subscribersByEventType.begin(); it != _subscribersByEventType.end(); ++it ) {
            SubscribersType item = *it;
            if (event->type() == item.first) {
                EventBusSubscriber *subscriber = item.second;
                if (!subscriber->sender || subscriber->sender == event->sender()) {
                    theReceivers.append(subscriber->receiver);

                    if (dis && dis != subscriber->dispatcher)
                        throw "You cannot publish events to subscribers from more than one thread.";

                    dis = subscriber->dispatcher;
                }
            }
        }

        if (dis)
            sendEvent(dis, new EventWrapper(event, theReceivers));
    }

    void EventBus::send(QObject *receiver, Event *event)
    {
        QMutexLocker lock(&_lock);

        if (!receiver)
            return;

        QThread *thread = receiver->thread();
        EventBusDispatcher *dis = dispatcher(thread);

        sendEvent(dis, new EventWrapper(event, receiver));
    }

    void EventBus::send(QList<QObject *> receivers, Event *event)
    {
        QMutexLocker lock(&_lock);

        if (receivers.count() == 0)
            return;

        QThread *thread = receivers.last()->thread();
        EventBusDispatcher *dis = dispatcher(thread);

        sendEvent(dis, new EventWrapper(event, receivers));
    }

    void EventBus::subscribe(QObject *receiver, QEvent::Type type, QObject *sender /* = NULL */)
    {
        QMutexLocker lock(&_lock);

        QThread *currentThread = QThread::currentThread();
        EventBusDispatcher *dis = dispatcher(currentThread);

        // subscribe to destroyed signal in order to remove
        // listener (receiver) from list of subscribers
        VERIFY(connect(receiver, SIGNAL(destroyed(QObject*)), this, SLOT(unsubscibe(QObject*))));

        // add subscriber
        _subscribersByEventType.push_back(SubscribersType(type, new EventBusSubscriber(dis, receiver, sender)));
    }

    void EventBus::unsubscibe(QObject *receiver)
    {
        QMutexLocker lock(&_lock);
        _subscribersByEventType.erase(std::remove_if(_subscribersByEventType.begin(), _subscribersByEventType.end(),
            RemoveIfReciver(receiver)), _subscribersByEventType.end());
    }

    /**
     * @brief Returns dispatcher for specified thread. If there is no dispatcher
     * for this thread registered, it will be created and moved to 'thread' thread.
     */
    EventBusDispatcher *EventBus::dispatcher(QThread *thread)
    {
        DispatchersContainerType::iterator disIt = std::find_if(
            _dispatchersByThread.begin(), _dispatchersByThread.end(), FindIfReciver(thread));
       
        if (disIt != _dispatchersByThread.end()) {
            return (*disIt).second;
        }
        else {
            EventBusDispatcher *dis = new EventBusDispatcher();
            dis->moveToThread(thread);
            _dispatchersByThread.push_back(DispatchersType(thread, dis));
            return dis;
        }        
    }

    /**
     * @brief Sends event synchronousely, if current thread and dispatcher thread are
     * the same. Sends asynchronousely, if this is cross-thread communication;
     */
    void EventBus::sendEvent(EventBusDispatcher *dispatcher, EventWrapper *wrapper)
    {
        if (dispatcher->thread() == QThread::currentThread()) {
            QCoreApplication::sendEvent(dispatcher, wrapper);
            delete wrapper;
        }
        else {
            QCoreApplication::postEvent(dispatcher, wrapper);
        }
    }

}
