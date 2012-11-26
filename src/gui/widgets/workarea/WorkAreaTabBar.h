#ifndef WORKAREATABBAR_H
#define WORKAREATABBAR_H

#include <QTabBar>

namespace Robomongo
{
    class WorkAreaTabBar : public QTabBar
    {
        Q_OBJECT

    public:

        WorkAreaTabBar(QWidget * parent = 0);

        void mouseReleaseEvent(QMouseEvent * event);

    };
}

#endif // WORKAREATABBAR_H
