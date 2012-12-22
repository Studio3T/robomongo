#include <QEvent>
#include <QObject>
#include <QCoreApplication>
#include "EventBus.h"
#include "EventBusSubscriber.h"
#include "events/MongoEvents.h"
#include <QThread>
#include "EventWrapper.h"
#include <QMutexLocker>

using namespace Robomongo;

class SomethingHappened;

EventBus::EventBus() : QObject()
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
    foreach(EventBusSubscriber *subscriber, subscribers)
    {
        if (!subscriber->sender || subscriber->sender == event->sender()) {
            //QCoreApplication::sendEvent(subscriber->receiver, event);
            //QGenericArgument arg = QGenericArgument("SomethingHappened*", event);

            theReceivers.append(subscriber->receiver);

            if (dis && dis != subscriber->dispatcher)
                throw "You cannot publish events to subscribers from more than one thread.";

            dis = subscriber->dispatcher;

            // was:
            // const char * typeName = event->typeString();
            // QMetaObject::invokeMethod(subscriber->receiver, "handle", QGenericArgument(typeName, &event));
        }
    }

    QCoreApplication::postEvent(dis,
        new EventWrapper(event, theReceivers));

    //    delete event;
}

void EventBus::send(QObject *receiver, Event *event)
{
    QMutexLocker lock(&_lock);

    if (!receiver)
        return;

    QThread *thread = receiver->thread();
    EventBusDispatcher *dis = dispatcher(thread);

    QList<QObject *> receivers;
    receivers << receiver;

    QCoreApplication::postEvent(dis,
        new EventWrapper(event, receivers));
}

void EventBus::send(QList<QObject *> receivers, Event *event)
{
    QMutexLocker lock(&_lock);

    if (receivers.count() == 0)
        return;

    QThread *thread = receivers.last()->thread();
    EventBusDispatcher *dis = dispatcher(thread);

    QCoreApplication::postEvent(dis,
        new EventWrapper(event, receivers));
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

void EventBus::unsubscibe(QObject *receiver)
{
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
