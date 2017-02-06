#include "robomongo/gui/widgets/workarea/WelcomeTab.h"

#include <QObject>
#include <QPushButton>
#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QPixmap>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDir>
#include <QDesktopServices>

#include "robomongo/core/utils/Logger.h"       
#include "robomongo/core/utils/QtUtils.h"

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
    QString const blog0 = "<p><h3><font color=\"#2d862d\"><a style = 'text-decoration:none'"
        "href=\"http://blog.robomongo.org/robomongo-1-rc1/\">"
        "Robomongo 1.0 RC1 brings support to Replica Set Clusters</h3></p>"
        "<font color=\"gray\">02 Feb 2017</font>";

    QString const blog1 = "<p><h3><a style = 'text-decoration:none'"
        "href=\"http://blog.robomongo.org/robomongo-1-rc1/\">"
        "Robomongo 0.9.0 Final</h3></p>"
        "<font color=\"gray\">06 Oct 2016</font>";

    QString const blog2 = "<p><h3><a style = 'text-decoration:none'"
        "href=\"http://blog.robomongo.org/robomongo-1-rc1/\">"
        "Robomongo RC10 brings support to SSL</h3></p>"
        "<font color=\"gray\">19 Aug 2016</font>";

    QString const blog3 = "<p><h3><a style = 'text-decoration:none'"
        "href=\"http://blog.robomongo.org/robomongo-1-rc1/\">"
        "Robomongo RC9</h3></p>"
        "<font color=\"gray\">01 Jun 2016</font>";

    QString const blog4 = "<p><h3><a style = 'text-decoration:none'"
        "href=\"http://blog.robomongo.org/robomongo-1-rc1/\">"
        "Robomongo RC8</h3></p>"
        "<font color=\"gray\">14 Apr 2016</font>";


    WelcomeTab::WelcomeTab(QWidget *parent) :
        QWidget(parent)
    {
        //setStyleSheet("background-color: yellow");    // todo: to be removed, debug only

        auto section1 = new QLabel(whatsNew + str1 + str2);
        section1->setTextInteractionFlags(Qt::TextSelectableByMouse);
        section1->setWordWrap(true);
        section1->setMinimumWidth(700);

        auto manager = new QNetworkAccessManager;
        VERIFY(connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*))));
        manager->get(QNetworkRequest(
            QUrl("http://blog.robomongo.org/content/images/2017/02/bottom.png")));

        _pic1 = new QLabel;
        //pic1->setPixmap(pic1URL);
        //_pic1->setMaximumSize(800,400);
        _pic1->setTextInteractionFlags(Qt::TextSelectableByMouse);
        //pic1->setStyleSheet("background-color: gray");

        auto section2 = new QLabel(blogPosts + blog0 + blog1 + blog2 + blog3 + blog4);
        section2->setTextInteractionFlags(Qt::TextSelectableByMouse);
        section2->setWordWrap(true);
        section2->setTextFormat(Qt::RichText);
        section2->setTextInteractionFlags(Qt::TextBrowserInteraction);
        section2->setOpenExternalLinks(true);

        auto buttonLay = new QHBoxLayout;
        _allBlogsButton = new QPushButton("All Blog Posts");
        //button->setStyleSheet("font-size: 14pt; background-color: green; border: 2px; color: white");
        _allBlogsButton->setStyleSheet("font: bold; color: green");
        VERIFY(connect(_allBlogsButton, SIGNAL(clicked()), this, SLOT(on_allBlogsButton_clicked())));
        _allBlogsButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        buttonLay->addWidget(_allBlogsButton);
        buttonLay->addStretch();

        auto mainLayout = new QVBoxLayout;
        mainLayout->setContentsMargins(20, -1, -1, -1);
        mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        mainLayout->addSpacing(30);
        mainLayout->addWidget(section1, 0 , Qt::AlignTop);
        mainLayout->addWidget(_pic1, 0, Qt::AlignTop);
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

    void WelcomeTab::downloadFinished(QNetworkReply* reply)
    {
        auto img = new QPixmap;
        img->loadFromData(reply->readAll());

        if (img->isNull()) {
            LOG_MSG("WelcomeTab: Failed to download from network. Reason: " + reply->errorString(),
                     mongo::logger::LogSeverity::Warning());
            return;
        }

        auto const& cacheDir = QString("%1/.config/robomongo/1.0/cache/").arg(QDir::homePath());
        if(!QDir(cacheDir).exists())
            QDir().mkdir(cacheDir);

        //img->save(cacheDir + "img.png");

        _pic1->setPixmap(*img);
        _pic1->setPixmap(img->scaled(800, 600, Qt::AspectRatioMode::KeepAspectRatio));
        
    }

    void WelcomeTab::on_allBlogsButton_clicked()
    {
        QDesktopServices::openUrl(QUrl("http://blog.robomongo.org/"));
    }

}
