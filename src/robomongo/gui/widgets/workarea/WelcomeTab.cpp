#include "robomongo/gui/widgets/workarea/WelcomeTab.h"

#include <QtWebEngineWidgets>

/* ----------------------- Welcome Tab (Windwos, macOS) --------------------- */
namespace Robomongo {

    WelcomeTab::WelcomeTab(QScrollArea *parent) :
        QWidget(parent), _parent(parent)
    {
        auto webView = new QWebEngineView(this);
        QUrl url{
            //"https://www.gismeteo.by/weather-minsk-4248/2-weeks/" 
            // "http://www.qt.io/"
            "http://files.studio3t.com/rm-feed_3t_io/1.4.3/index.html"
        };
        webView->load(url);
        webView->show();

        auto mainLayout = new QHBoxLayout;
        mainLayout->setContentsMargins(-10, -10, -1, -1);
        mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        mainLayout->addWidget(webView);
        setLayout(mainLayout);

        webView->setStyleSheet("background-color: red");        // no effect
        //setStyleSheet("background-color: green");
        //mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
    }

}