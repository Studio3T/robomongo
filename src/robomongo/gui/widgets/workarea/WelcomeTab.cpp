#include "robomongo/gui/widgets/workarea/WelcomeTab.h"

#include <QObject>
#include <QPushButton>
#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QPixmap>


namespace Robomongo
{
    QString const recentConnections = "<p><h1><font color=\"#2d862d\">Recent Connections</h1></p>";

    QString const whatsNew = "<p><h1><font color=\"#2d862d\">What's New</h1></p>";
    QString const str1 = "<h3>Replica set status aka \"rs.status()\" shortcut menu item </h3>";
    QString const str2 = "With this new feature it is possible to check replica set status even" 
        " in offline mode (primary is unreachable) if there is at least one reachable member."
        " In the example shown below, we can get all information about the health of replica set"
        " and members with right click on replica set folder.";

    QString const blogPosts = "<p><h1><font color=\"#2d862d\">Blog Posts</h1></p>";
    QString const blog0 = "<p><h3><a href=\"http://blog.robomongo.org/robomongo-1-rc1/\">"
        "Robomongo 1.0 RC1 brings support to Replica Set Clusters</h3></p>"
        "<font color=\"gray\">02 Feb 2017</font>";

    QString const blog1 = "<p><h3><a href=\"http://blog.robomongo.org/robomongo-1-rc1/\">"
        "Robomongo 0.9.0 Final</h3></p>"
        "<font color=\"gray\">06 Oct 2016</font>";

    QString const blog2 = "<p><h3><a href=\"http://blog.robomongo.org/robomongo-1-rc1/\">"
        "Robomongo RC10 brings support to SSL</h3></p>"
        "<font color=\"gray\">19 Ayg 2016</font>";

    QString const blog3 = "<p><h3><a href=\"http://blog.robomongo.org/robomongo-1-rc1/\">"
        "Robomongo RC9</h3></p>"
        "<font color=\"gray\">01 Jun 2016</font>";

    QString const blog4 = "<p><h3><a href=\"http://blog.robomongo.org/robomongo-1-rc1/\">"
        "Robomongo RC8</h3></p>"
        "<font color=\"gray\">14 Apr 2016</font>";


    WelcomeTab::WelcomeTab(QWidget *parent) :
        QWidget(parent)
    {
        //setStyleSheet("background-color: yellow");    // todo: to be removed, debug only

        auto section1 = new QLabel(whatsNew + str1 + str2);
        section1->setTextInteractionFlags(Qt::TextSelectableByMouse);
        section1->setWordWrap(true);

        auto section2 = new QLabel(blogPosts + blog0 + blog1 + blog2 + blog3 + blog4);
        section2->setTextInteractionFlags(Qt::TextSelectableByMouse);
        section2->setWordWrap(true);
        section2->setTextFormat(Qt::RichText);
        section2->setTextInteractionFlags(Qt::TextBrowserInteraction);
        section2->setOpenExternalLinks(true);

        auto buttonLay = new QHBoxLayout;
        auto button = new QPushButton("Blog Posts' Main Page");
        button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        buttonLay->addWidget(button);
        buttonLay->addStretch();

        auto mainLayout = new QVBoxLayout;
        mainLayout->setContentsMargins(20, -1, -1, -1);
        mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        mainLayout->addSpacing(30);
        mainLayout->addWidget(section1, 0 , Qt::AlignTop);
        //mainLayout->addWidget(pic1, 0, Qt::AlignTop);
        mainLayout->addSpacing(30);
        mainLayout->addWidget(section2, 0, Qt::AlignTop);
        mainLayout->addSpacing(15);
        mainLayout->addLayout(buttonLay);
        mainLayout->addStretch();
        setLayout(mainLayout);
    }

    WelcomeTab::~WelcomeTab()
    {

    }

}
