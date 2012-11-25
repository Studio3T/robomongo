#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <QEvent>
#include <QMultiHash>

namespace Robomongo
{
    class Subscriber
    {
    public:
        explicit Subscriber(QObject *receiver, QObject *sender = NULL) :
            receiver(receiver),
            sender(sender) {}

        QObject *receiver;
        QObject *sender;
    };

    class Dispatcher : QObject
    {
        Q_OBJECT

    public:
        Dispatcher();

        /**
         * @brief Publishes event
         * @param sender - object, that emits this event
         */
        void publish(QObject *sender, QEvent *event);

        /**
         * @brief Subscribe to specified event
         */
        void subscribe(QObject *receiver, QEvent::Type type, QObject *sender = NULL);

    public slots:

        void unsubscibe(QObject *receiver);

    private:

        QMultiHash<QEvent::Type, Subscriber *> _subscribersByEventType;
    };
}

#endif // DISPATCHER_H
