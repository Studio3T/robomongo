#include "WorkAreaTabBar.h"
#include "QMouseEvent"

using namespace Robomongo;

WorkAreaTabBar::WorkAreaTabBar(QWidget *parent) : QTabBar(parent)
{
    setStyleSheet("QTabBar::close-button { "
                      "image: url(:/robomongo/icons/close_16x16.png);"
                      "width: 13px;"
                      "height: 13px;"
                  "}"
                  "QTabBar::close-button:hover { "
                                        "image: url(:/robomongo/icons/close_hover_16x16.png);"
                                        "width: 15px;"
                                        "height: 15px;"
                                    "}"
    );

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
