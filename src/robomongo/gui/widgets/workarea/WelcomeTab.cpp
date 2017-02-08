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
#include <QScrollArea>
#include <QScrollBar>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/utils/Logger.h"       
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    QString const recentConnections = "<p><h1><font color=\"#2d862d\">Recent Connections</h1></p>";

    QString const connLabelTemplate = "<p><a style='font-size:14px; color: #106CD6; text-decoration:none'"
                                      "href='%1'>%2</a></p>";
    
    QString const whatsNew = "<p><h1><font color=\"#2d862d\">What's New</h1></p>";
    QString const str1 = "<h3>Replica set status aka \"rs.status()\" shortcut menu item </h3>";
    QString const str2 = "With this new feature it is possible to check replica set status even" 
        " in offline mode (primary is unreachable) if there is at least one reachable member."
        " In the example shown below, we can get all information about the health of replica set"
        " and members with right click on replica set folder.";

    QString const blogPosts = "<p><h1><font color=\"#2d862d\">Blog Posts</h1></p>";

    QString const blog = "<p><a style = 'font-size:14px; color: #106CD6; text-decoration:none'"
        "href='%1'>%2</p>""<font color=\"gray\">%3</font>";

    QString const blog0 = blog.arg("http://blog.robomongo.org/robomongo-1-rc1/", 
        "Robomongo 1.0 RC1 brings support to Replica Set Clusters", "02 Feb 2017");

    QString const blog1 = blog.arg("http://blog.robomongo.org/robomongo-1-rc1/",
        "Robomongo 0.9.0 Final", "06 Oct 2016");

    QString const blog2 = blog.arg("http://blog.robomongo.org/robomongo-1-rc1/",
        "Robomongo RC10 brings support to SSL", "19 Aug 2016");

    QString const blog3 = blog.arg("http://blog.robomongo.org/robomongo-1-rc1/",
        "Robomongo RC9", "01 Jun 2016");

    QString const blog4 = blog.arg("http://blog.robomongo.org/robomongo-1-rc1/",
        "Robomongo RC8", "14 Apr 2016");

    // todo: move to in-class

    WelcomeTab::WelcomeTab(QScrollArea *parent) :
        QWidget(parent), _parent(parent)
    {
        //setStyleSheet("background-color: yellow");    // todo: to be removed, debug only

        AppRegistry::instance().bus()->subscribe(this, ConnectionEstablishedEvent::Type);

        _recentConnsLay = new QVBoxLayout;       
        auto recentConnLabel = new QLabel(recentConnections);

        auto clearButtonLay = new QHBoxLayout;
        _clearButton = new QPushButton("Clear Recent Connections");
        if (/*no recent connections*/1)
            _clearButton->setDisabled(true);
        else 
            _clearButton->setStyleSheet("color: #106CD6");

        VERIFY(connect(_clearButton, SIGNAL(clicked()), this, SLOT(on_clearButton_clicked())));
        _clearButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        clearButtonLay->addWidget(_clearButton);
        clearButtonLay->addStretch();

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
        _allBlogsButton->setStyleSheet("color: #106CD6");
        VERIFY(connect(_allBlogsButton, SIGNAL(clicked()), this, SLOT(on_allBlogsButton_clicked())));
        _allBlogsButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        buttonLay->addWidget(_allBlogsButton);
        buttonLay->addStretch();

        auto mainLayout = new QVBoxLayout;
        mainLayout->setContentsMargins(20, -1, -1, -1);
        mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        mainLayout->addSpacing(30);
        mainLayout->addWidget(recentConnLabel, 0, Qt::AlignTop);
        mainLayout->addLayout(_recentConnsLay);
        mainLayout->addSpacing(10);
        mainLayout->addLayout(clearButtonLay);
        mainLayout->addSpacing(30);
        mainLayout->addWidget(section1, 0, Qt::AlignTop);
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

    
    void WelcomeTab::on_clearButton_clicked()
    {
        while (auto item = _recentConnsLay->takeAt(0)) 
            delete item->widget();
        
        _clearButton->setDisabled(true);
        _clearButton->setFocus();
        _clearButton->setStyleSheet("");
        
        _parent->verticalScrollBar()->setValue(0);
    }

    void WelcomeTab::linkActivated(QString const& link)
    {
        for (auto const conn : AppRegistry::instance().settingsManager()->connections()) {
            if (QString::number(conn->uniqueId()) == link) {
                AppRegistry::instance().app()->openServer(conn, ConnectionPrimary);
                return;
            }
        }
    }

    void WelcomeTab::linkHovered(QString const& link)
    {
        //setStyleSheet("QLabel { color: green; text-decoration: underline; }");        
    }

    void WelcomeTab::handle(ConnectionEstablishedEvent *event)
    {
        // Do not make UI changes for non PRIMARY connections
        if (event->connectionType != ConnectionPrimary)
            return;

        auto conn = AppRegistry::instance().settingsManager()
            ->getConnectionSettings(event->connInfo._originalConnectionSettingsId);

        //// Quick duplicate detection
        //if (conn->uniqueId() == _lastAddedConnId)
        //    return;

        auto xx = _recentConnsLay->count();

        // Duplicate detection
        for (int i = 0; i < _recentConnsLay->count(); ++i) {
            auto label = dynamic_cast<QLabel*>(_recentConnsLay->itemAt(i)->widget());
            if (label) {
                // todo:
                //"href='uniqueID'>%2</a></p>");
                auto const unqID = label->text().split("href='")[1].split("'")[0].toInt();
                if (unqID == conn->uniqueId())
                    return;
            }
        }


        // Add conn into recent conns list
        auto connLabel = new QLabel(connLabelTemplate.arg(QString::number(conn->uniqueId()),
            QString::fromStdString(conn->connectionName() + " (" + conn->hostAndPort().toString() + ')')));

        connLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        VERIFY(connect(connLabel, SIGNAL(linkActivated(QString)), this, SLOT(linkActivated(QString))));
        VERIFY(connect(connLabel, SIGNAL(linkHovered(QString)), this, SLOT(linkHovered(QString))));
        _recentConnsLay->insertWidget(0, connLabel);
        _clearButton->setEnabled(true);
        _clearButton->setStyleSheet("color: #106CD6");
        _lastAddedConnId = conn->uniqueId();
    }
}
