#include "WorkAreaTabBar.h"
#include "QMouseEvent"
#include <QStylePainter>
#include <QStyleOptionTabV3>

using namespace Robomongo;

WorkAreaTabBar::WorkAreaTabBar(QWidget *parent) : QTabBar(parent)
{
    int a = 456;
    QString styles =
        "QTabBar::close-button { "
            "image: url(:/robomongo/icons/close_2_16x16.png);"
            "width: 10px;"
            "height: 10px;"
        "}"
        "QTabBar::close-button:hover { "
              "image: url(:/robomongo/icons/close_hover_16x16.png);"
              "width: 15px;"
              "height: 15px;"
        "}"
        "QTabBar::tab {"
            "background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
                                        "stop: 0 #F0F0F0, stop: 0.4 #DEDEDE,"
                                        "stop: 0.5 #E6E6E6, stop: 1.0 #E1E1E1);"
            "border: 1px solid #C4C4C3;"
            "border-bottom-color: #C2C7CB;" // same as the pane color
            "border-top-left-radius: 6px;"
            "border-top-right-radius: 6px;"
            "min-width: 8ex;"
            "max-width: 200px;"
            "padding: 4px 0px 4px 2px;"
            "margin: 0px;"
            "margin-right: -2px;"
        "}"

        "QTabBar::tab:selected, QTabBar::tab:hover {"
            "background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
                                        "stop: 0 #fafafa, stop: 0.4 #f4f4f4,"
                                        "stop: 0.5 #e7e7e7, stop: 1.0 #fafafa);"
        "}"

        "QTabBar::tab:selected {"
            "border-color: #9B9B9B;"
            "border-bottom-color: #fafafa;"
        "}"

        "QTabBar::tab:!selected {"
            "margin-top: 2px;" // make non-selected tabs look smaller
        "}  "
    ;

    setStyleSheet(styles);
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
