#include <QEvent>
#include <QObject>
#include <QCoreApplication>
#include "Dispatcher.h"

using namespace Robomongo;

Dispatcher::Dispatcher()
{
}

void Dispatcher::publish(QObject *sender, QEvent *event)
{
    QList<QObject *> receivers = _receiversByEventType.values(event->type());
    foreach(QObject *receiver, receivers)
    {
        QCoreApplication::sendEvent(receiver, event);
    }

    QMultiHash<QEvent::Type, QObject *> map = _receiversByEventTypeBySender.value(sender);
    QList<QObject *> receivers2 = map.values(event->type());
    foreach(QObject *receiver2, receivers2) {
        QCoreApplication::sendEvent(receiver2, event);
    }
}


void Dispatcher::subscribe(QObject *receiver, QEvent::Type type, QObject *sender)
{
    if (!sender)
    {
        _receiversByEventType.insert(type, receiver);
        return;
    }

    if (!_receiversByEventTypeBySender.contains(sender))
        _receiversByEventTypeBySender.insert(sender, QMultiHash<QEvent::Type, QObject *>());

    QMultiHash<QEvent::Type, QObject *> map = _receiversByEventTypeBySender.value(sender);
    map.insert(type, receiver);
}
