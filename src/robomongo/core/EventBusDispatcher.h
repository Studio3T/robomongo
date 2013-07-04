#pragma once
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
    };
}
