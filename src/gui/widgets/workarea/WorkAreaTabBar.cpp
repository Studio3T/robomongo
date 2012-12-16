#include "WorkAreaTabBar.h"
#include "QMouseEvent"

using namespace Robomongo;

WorkAreaTabBar::WorkAreaTabBar(QWidget *parent) : QTabBar(parent)
{
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
              "subcontrol-position: right"
        "}"

        #ifdef Q_OS_WIN32
        "QTabBar::tab {"
            "background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
                                        "stop: 0 #E1E1E1, stop: 0.4 #DDDDDD,"
                                        "stop: 0.5 #D8D8D8, stop: 1.0 #D3D3D3);"
            "border: 1px solid #C4C4C3;"
            "border-bottom-color: #C2C7CB; /* same as the pane color */"
            "border-top-left-radius: 8px;"
            "border-top-right-radius: 8px;"
            "min-width: 8ex;"
            "padding: 5px;"
            "margin-right: 0px;"
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
            "margin-top: 2px; /* make non-selected tabs look smaller */"
        "}                "
        #endif
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
