#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <QEvent>
#include <QMultiHash>

namespace Robomongo
{
    class Dispatcher
    {
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

    private:

        QMultiHash<QEvent::Type, QObject *> _receiversByEventType;

        QHash<QObject *,
            QMultiHash<QEvent::Type, QObject *> > _receiversByEventTypeBySender;
    };
}

#endif // DISPATCHER_H
