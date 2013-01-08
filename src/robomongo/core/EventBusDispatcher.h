#ifndef EVENTBUSDISPATCHER_H
#define EVENTBUSDISPATCHER_H

#include <QObject>

namespace Robomongo
{
    /**
     * @brief The EventBusDispatcher class
     */
    class EventBusDispatcher : public QObject
    {
        Q_OBJECT
    public:

        EventBusDispatcher(QObject *parent = 0);

    protected:

        bool event(QEvent *qevent);

    signals:

    public slots:

    };
}

#endif // EVENTBUSDISPATCHER_H
