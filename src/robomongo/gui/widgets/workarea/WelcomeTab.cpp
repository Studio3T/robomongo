#include "robomongo/gui/widgets/workarea/WelcomeTab.h"

#include <QtWebEngineWidgets>

/* ----------------------- Welcome Tab (Windwos, macOS) --------------------- */
namespace Robomongo {

    WelcomeTab::WelcomeTab(QScrollArea *parent) :
        QWidget(parent), _parent(parent)
    {
        auto webView = new QWebEngineView(this);
        QUrl url{
            "http://files.studio3t.com/rm-feed_3t_io/1.4.3/index.html"
        };
        webView->load(url);
        webView->show();

        auto mainLayout = new QHBoxLayout;
        mainLayout->setContentsMargins(-10, -10, -1, -1);
        mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        mainLayout->addWidget(webView);
        setLayout(mainLayout);
    }

}