#include "robomongo/core/EventBus.h"

#include <QEvent>
#include <QObject>
#include <QCoreApplication>
#include <QThread>
#include <QMutexLocker>

#include "robomongo/core/EventBusSubscriber.h"
#include "robomongo/core/Event.h"
#include "robomongo/core/EventWrapper.h"

using namespace Robomongo;

class SomethingHappened;

EventBus::EventBus() : QObject(),
    _lock(QMutex::Recursive)
{
}

EventBus::~EventBus()
{
    QHashIterator<QThread *, EventBusDispatcher *> i(_dispatchersByThread);

    while(i.hasNext()) {
        i.next();
        delete i.value();
    }
}

void EventBus::publish(Event *event)
{
    QMutexLocker lock(&_lock);

    QList<EventBusSubscriber *> subscribers = _subscribersByEventType.values(event->type());
    QList<QObject*> theReceivers;
    EventBusDispatcher *dis = NULL;
	for(QList<EventBusSubscriber *>::iterator it = subscribers.begin();it!=subscribers.end();++it)
    {
		EventBusSubscriber *subscriber = *it;
        if (!subscriber->sender || subscriber->sender == event->sender()) {
            theReceivers.append(subscriber->receiver);

            if (dis && dis != subscriber->dispatcher)
                throw "You cannot publish events to subscribers from more than one thread.";

            dis = subscriber->dispatcher;
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
    connect(receiver, SIGNAL(destroyed(QObject*)), this, SLOT(unsubscibe(QObject*)));

    // add subscriber
    _subscribersByEventType.insert(type, new EventBusSubscriber(dis, receiver, sender));
}

void EventBus::unsubscibe(QObject *receiver)
{
    QMutexLocker lock(&_lock);

    QString name = receiver->objectName();
    QString cname = receiver->metaObject()->className();

    QMutableHashIterator<QEvent::Type, EventBusSubscriber *> i(_subscribersByEventType);

    while(i.hasNext()) {
        i.next();
        if (i.value()->receiver == receiver) {
            delete i.value();
            i.remove();
        }
    }
}

/**
 * @brief Returns dispatcher for specified thread. If there is no dispatcher
 * for this thread registered, it will be created and moved to 'thread' thread.
 */
EventBusDispatcher *EventBus::dispatcher(QThread *thread)
{
    EventBusDispatcher *dis = _dispatchersByThread.value(thread);

    if (!dis) {
        dis = new EventBusDispatcher();
        dis->moveToThread(thread);
        _dispatchersByThread.insert(thread, dis);
    }

    return dis;
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
    } else {
        QCoreApplication::postEvent(dispatcher, wrapper);
    }
}


