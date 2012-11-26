#include "WorkAreaTabBar.h"
#include "QMouseEvent"

using namespace Robomongo;

WorkAreaTabBar::WorkAreaTabBar(QWidget *parent) : QTabBar(parent)
{
}

void WorkAreaTabBar::mouseReleaseEvent(QMouseEvent * event)
{
    if (event->button() == Qt::MidButton)
    {
        // * int QTabBar::tabAt ( const QPoint & position ) const
        // * Returns the index of the tab that covers position or -1 if no tab covers position;
        int tabIndex = tabAt(event->pos());

        if (tabIndex < 0)
            return;

        emit tabCloseRequested(tabIndex);
    }

    QTabBar::mouseReleaseEvent(event);
}
