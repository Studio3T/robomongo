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
#include <QFile>
#include <QFileInfo>
#include <QDesktopServices>
#include <QScrollArea>
#include <QScrollBar>
#include <QEvent>
#include <QXmlStreamReader>
#include <QMessageBox>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/utils/Logger.h"       
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/utils/common.h"

namespace Robomongo
{

    QString const Text1_LastModifiedDateKey("wtText1LastModifiedDate");
    QString const Image1_LastModifiedDateKey("wtImage1LastModifiedDate");
    QString const Rss_LastModifiedDateKey("wtRssLastModifiedDate");

    QString const RssFileName = "rss.xml";

    // todo: make generic, move to a header 
    //auto const& CacheDir = QString("%1/.config/robomongo/1.0.0/cache/").arg(QDir::homePath());    // todo

    std::unique_ptr<QFile> loadFileFromCache(QString const& fileName)
    {
        QFileInfo check_file(CacheDir + fileName);
        if (check_file.exists() && check_file.isFile()) {   // Use cached file
            std::unique_ptr<QFile> file(new QFile(CacheDir + fileName));
            if (file->open(QFile::ReadOnly | QFile::Text)) {
                return file;
            }
        }
        return nullptr;
    }

    bool saveIntoCache(QString const& fileName, QString const& fileData, 
                       QString const& lastModifiedKey, QString const& lastModifiedDate)
    {
        // Save file into cache directory
        if (!QDir(CacheDir).exists())
            QDir().mkdir(CacheDir);
        else {  // cache dir. exists            
            QFileInfo check_file(CacheDir + fileName);
            // Make sure we delete the old file in order to cache the newly downloaded file
            if (check_file.exists() && check_file.isFile()) {
                if (!QFile::remove(CacheDir + fileName)) {
                    LOG_MSG("WelcomeTab: Failed to delete cached file at: " + CacheDir + fileName,
                        mongo::logger::LogSeverity::Warning());
                    return false;
                }
            }
        }
        QFile file(CacheDir + fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            // todo: log
            return false;
        }
        QTextStream out(&file);
        out << fileData;

        // Save file's last modified date into settings
        AppRegistry::instance().settingsManager()->addCacheData(lastModifiedKey, lastModifiedDate);
        AppRegistry::instance().settingsManager()->save();
        return true;
    }

    bool saveIntoCache(QString const& fileName, QPixmap const& pixMap, 
                       QString const& lastModifiedKey, QString const& lastModifiedDate)
    {
        // Save file into cache directory
        if (!QDir(CacheDir).exists())
            QDir().mkdir(CacheDir);
        else {  // cache dir. exists            
            QFileInfo check_file(CacheDir + fileName);
            // Make sure we delete the old file in order to cache the newly downloaded file
            if (check_file.exists() && check_file.isFile()) {
                if (!QFile::remove(CacheDir + fileName)) {
                    LOG_MSG("WelcomeTab: Failed to delete cached file at: " + CacheDir + fileName,
                        mongo::logger::LogSeverity::Warning());
                    return false;
                }
            }
        }
        QFile file(CacheDir + fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            // todo: log
            return false;
        }
        pixMap.save(CacheDir + fileName);

        // Save file's last modified date into settings
        AppRegistry::instance().settingsManager()->addCacheData(lastModifiedKey, lastModifiedDate);
        AppRegistry::instance().settingsManager()->save();
        return true;
    }

    bool saveIntoCache(QString const& fileName, QByteArray* data, 
                       QString const& lastModifiedKey, QString const& lastModifiedDate)
    {
        // Save file into cache directory
        if (!QDir(CacheDir).exists())
            QDir().mkdir(CacheDir);
        else {  // cache dir. exists            
            QFileInfo check_file(CacheDir + fileName);
            // Make sure we delete the old file in order to cache the newly downloaded file
            if (check_file.exists() && check_file.isFile()) {
                if (!QFile::remove(CacheDir + fileName)) {
                    LOG_MSG("WelcomeTab: Failed to delete cached file at: " + CacheDir + fileName,
                        mongo::logger::LogSeverity::Warning());
                    return false;
                }
            }
        }
        QFile file(CacheDir + fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            // todo: log
            return false;
        }
        file.write(*data);

        // Save file's last modified date into settings
        AppRegistry::instance().settingsManager()->addCacheData(lastModifiedKey, lastModifiedDate);
        AppRegistry::instance().settingsManager()->save();
        return true;
    }

    struct BlogInfo
    {
        BlogInfo(QString const& title, QString const& link, QString const& publishDate)
            : title(title), link(link), publishDate(publishDate) {}

        QString const title;
        QString const link;
        QString const publishDate;
    };

    struct BlogLinkLabel : public QLabel
    {
        BlogLinkLabel(QString const& args) 
            : QLabel(args) {}
    };

    /* todo: brief: Special button to use on the right side of a recent connection label */
    struct CustomButton : public QPushButton
    {
        void setLabelOnLeft(QLabel* lab) { label = lab; }
        void setParentLayout(QLayout* lay) { parentLayout = lay; }

        QLabel* label = nullptr;
        QLayout* parentLayout = nullptr;
    };

    // todo: brief
    std::once_flag onceFlag1;
    int CustomButtonHeight = -1;

    QString const recentConnectionsStr = "<p><h1><font color=\"#2d862d\">Recent Connections</h1></p>";

    QString const connLabelTemplate = "<p><a style='font-size:14px; color: #106CD6; text-decoration: none;'"
                                      "href='%1'>%2</a></p>";
    
    QString const whatsNew = "<p><h1><font color=\"#2d862d\">What's New</h1></p>";

    QString const BlogsHeader = "<p><h1><font color=\"#2d862d\">Blog Posts</h1></p>";

    QString const BlogLinkTemplate = "<p><a style = 'font-size:14px; color: #106CD6; text-decoration: none;'"
                                     "href='%1'>%2</a></p>";

    /*
    * http://files.studio3t.com/robo/1/image.png (for development)
    * http://rm-wn.3t.io/robo/1/image.png (when shipping)
    */
    QUrl const Pic1_URL = QString("https://rm-feed.3t.io/1/image.png");
    
    /*
    * http://files.studio3t.com/robo/1/contents.txt (for development)
    * http://rm-wn.3t.io/robo/1/contents.txt (when shipping)
    */
    QUrl const Text1_URL = QString("https://rm-feed.3t.io/1/contents.txt");


    WelcomeTab::WelcomeTab(QScrollArea *parent) :
        QWidget(parent), _parent(parent)
    {
        AppRegistry::instance().bus()->subscribe(this, ConnectionEstablishedEvent::Type);

        auto const settingsManager = AppRegistry::instance().settingsManager();

        _recentConnsLay = new QVBoxLayout;       
        auto recentConnLabel = new QLabel(recentConnectionsStr);

        auto clearButtonLay = new QHBoxLayout;
        _clearButton = new QPushButton("Clear Recent Connections");

        // Load and add recent connections from settings
        auto const& recentConnections = AppRegistry::instance().settingsManager()->recentConnections();
        if (recentConnections.size() < 1) 
            _clearButton->setDisabled(true);
        else {
            _clearButton->setStyleSheet("color: #106CD6");
            for (auto const& rconn : recentConnections) {
                ConnectionSettings const* conn = settingsManager->getConnectionSettingsByUuid(rconn.uuid);
                addRecentConnectionItem(conn, false);
            }
        }

        VERIFY(connect(_clearButton, SIGNAL(clicked()), this, SLOT(on_clearButton_clicked())));
        //_clearButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        clearButtonLay->addWidget(_clearButton);
        clearButtonLay->addStretch();

        //// What's new section
        _whatsNewHeader = new QLabel(whatsNew);
        _whatsNewText = new QLabel;
        _whatsNewText->setTextInteractionFlags(Qt::TextSelectableByMouse);
        _whatsNewText->setTextFormat(Qt::RichText);
        _whatsNewText->setTextInteractionFlags(Qt::TextBrowserInteraction);
        _whatsNewText->setOpenExternalLinks(true);
        _whatsNewText->setWordWrap(true);
        _whatsNewText->setMinimumWidth(750);
        _whatsNewText->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        //_whatsNewText->setMinimumHeight(_whatsNewText->sizeHint().height());

        auto text1Downloader = new QNetworkAccessManager;
        VERIFY(connect(text1Downloader, SIGNAL(finished(QNetworkReply*)), this, SLOT(on_downloadTextReply(QNetworkReply*))));
        text1Downloader->head(QNetworkRequest(Text1_URL));

        _pic1 = new QLabel;
        //pic1->setPixmap(pic1URL);
        //_pic1->setMaximumSize(800,400);
        _pic1->setTextInteractionFlags(Qt::TextSelectableByMouse);
        //pic1->setStyleSheet("background-color: gray");
        _pic1->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

        auto pic1Downloader = new QNetworkAccessManager;
        VERIFY(connect(pic1Downloader, SIGNAL(finished(QNetworkReply*)), 
               this, SLOT(on_downloadPictureReply(QNetworkReply*))));
        pic1Downloader->head(QNetworkRequest(Pic1_URL));

        _blogsSection = new QLabel;
        _blogsSection->setTextInteractionFlags(Qt::TextSelectableByMouse);
        //_blogsSection->setWordWrap(true);
        _blogsSection->setTextFormat(Qt::RichText);
        _blogsSection->setTextInteractionFlags(Qt::TextBrowserInteraction);
        _blogsSection->setOpenExternalLinks(true);

        auto rssDownloader = new QNetworkAccessManager;
        VERIFY(connect(rssDownloader, SIGNAL(finished(QNetworkReply*)), this, SLOT(on_downloadRssReply(QNetworkReply*))));
        QUrl const rssURL = QString("http://blog.robomongo.org/rss/");
        rssDownloader->get(QNetworkRequest(rssURL));

        auto buttonLay = new QHBoxLayout;
        _allBlogsButton = new QPushButton("All Blog Posts");
        //button->setStyleSheet("font-size: 14pt; background-color: green; border: 2px; color: white");        
        _allBlogsButton->setStyleSheet("color: #106CD6");
        VERIFY(connect(_allBlogsButton, SIGNAL(clicked()), this, SLOT(on_allBlogsButton_clicked())));
        _allBlogsButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        buttonLay->addWidget(_allBlogsButton);
        buttonLay->addStretch();

        _blogLinksLay = new QVBoxLayout;
        _blogLinksLay->setAlignment(Qt::AlignLeft);
        //_blogLinksLay->setSpacing(0);

        auto rightLayout = new QVBoxLayout;
        rightLayout->setContentsMargins(20, -1, -1, -1);
        rightLayout->addWidget(new QLabel(BlogsHeader), 0, Qt::AlignTop);
        rightLayout->addLayout(_blogLinksLay);
        rightLayout->addSpacing(15);
        rightLayout->addLayout(buttonLay);
        rightLayout->addStretch();

        auto leftLayout = new QVBoxLayout;
        leftLayout->addWidget(recentConnLabel, 0, Qt::AlignTop);
        leftLayout->addLayout(_recentConnsLay);
        leftLayout->addSpacing(10);
        leftLayout->addLayout(clearButtonLay);
        //leftLayout->addStretch();
        leftLayout->addSpacing(20);
        leftLayout->addWidget(_whatsNewHeader, 0, Qt::AlignTop);
        leftLayout->addWidget(_pic1, 0, Qt::AlignTop);
        leftLayout->addWidget(_whatsNewText, 0, Qt::AlignTop);
        leftLayout->setSizeConstraint(QLayout::SetMinimumSize);

        auto mainLayout = new QHBoxLayout;
        mainLayout->setContentsMargins(20, 20, -1, -1);
        mainLayout->addLayout(leftLayout);
        mainLayout->addSpacing(20);
        mainLayout->addLayout(rightLayout);
        //mainLayout->addStretch();
        mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        setLayout(mainLayout);

        //setStyleSheet("background-color: yellow");
    }

    WelcomeTab::~WelcomeTab()
    {

    }

    void WelcomeTab::on_downloadTextReply(QNetworkReply* reply)
    {
        if (reply->operation() == QNetworkAccessManager::HeadOperation) {
            if (reply->error() == QNetworkReply::NoError) { // No network error
                QString const& createDate = reply->header(QNetworkRequest::LastModifiedHeader).toString();
                if (createDate == AppRegistry::instance().settingsManager()->cacheData(Text1_LastModifiedDateKey)
                    && fileExists(CacheDir + Text1_URL.fileName())) 
                {
                    // Load from cache
                    auto file = loadFileFromCache(Text1_URL.fileName());
                    QTextStream in(file.get());
                    QString str(in.readAll());
                    _whatsNewText->setText(str);
                    return;
                }
                else {  // Get from internet
                    reply->manager()->get(QNetworkRequest(Text1_URL));
                }
            }
            else {  // There is a network error 
                // Load from cache
                auto file = loadFileFromCache(Text1_URL.fileName());
                QTextStream in(file.get());
                QString str(in.readAll());
                _whatsNewText->setText(str);
                return;
            }
        }
        else if (reply->operation() == QNetworkAccessManager::GetOperation) {
            // todo: handle get operation fails
            QString str(QUrl::fromPercentEncoding(reply->readAll()));
            if (str.isEmpty()) {
                LOG_MSG("WelcomeTab: Failed to download text file from URL. Reason: " + reply->errorString(),
                    mongo::logger::LogSeverity::Warning());
                // todo: load from cache?
                return;
            }
            _whatsNewText->setText(str);
            saveIntoCache(Text1_URL.fileName(), str, Text1_LastModifiedDateKey, 
                          reply->header(QNetworkRequest::LastModifiedHeader).toString());
        }
    }

    void WelcomeTab::on_downloadPictureReply(QNetworkReply* reply)
    {
        QPixmap image;
        auto const ImageToTextRatio = 0.9;  // todo: make static

        if (reply->error() != QNetworkReply::NoError) { // Network error, load from cache
            image = QPixmap(CacheDir + Pic1_URL.fileName());
            _pic1->setPixmap(image.scaledToWidth(_whatsNewText->width()*ImageToTextRatio));
            return;
        }

        // No network error
        if (reply->operation() == QNetworkAccessManager::HeadOperation) {
                QString const& createDate = reply->header(QNetworkRequest::LastModifiedHeader).toString();
                // If the file in URL is not newer load from cache, otherwise get from internet
                if (createDate == AppRegistry::instance().settingsManager()->cacheData(Image1_LastModifiedDateKey)
                    && fileExists(CacheDir + Pic1_URL.fileName())) 
                {
                    image = QPixmap(CacheDir + Pic1_URL.fileName());    // Load from cache
                }
                else {   // Get from internet
                    reply->manager()->get(QNetworkRequest(Pic1_URL));
                    return;
                }
        }
        else if (reply->operation() == QNetworkAccessManager::GetOperation) {
            image.loadFromData(reply->readAll());

            if (image.isNull()) {
                LOG_MSG("WelcomeTab: Failed to download image file from internet. Reason: " + reply->errorString(),
                         mongo::logger::LogSeverity::Warning());
                image = QPixmap(CacheDir + Pic1_URL.fileName());
            }
            else {
                saveIntoCache(Pic1_URL.fileName(), image, Image1_LastModifiedDateKey,
                              reply->header(QNetworkRequest::LastModifiedHeader).toString());
            }
        }

        // Set the image
        _pic1->setPixmap(image.scaledToWidth(_whatsNewText->width()*ImageToTextRatio));
    }

    void WelcomeTab::on_downloadRssReply(QNetworkReply* reply)
    {
        // todo: if reply->error(), log and return
        // todo: load from cache if download fails

        QByteArray data = reply->readAll();
        if (data.isEmpty() || reply->error() != QNetworkReply::NoError) {
            // Load from cache
            if (auto file = loadFileFromCache(RssFileName)) 
                data = file->readAll();
        }
        
        QXmlStreamReader xmlReader(data);

        int count = 0;
        int const MaxBlogCountShown = 10;
        QString title, link, pubDate;
        while (!xmlReader.atEnd()) {
            xmlReader.readNext();
            if (xmlReader.isStartElement()) {
                if (xmlReader.name() == "title") 
                    title = xmlReader.readElementText();
                else if (xmlReader.name() == "link")
                    link = xmlReader.readElementText();
                else if (xmlReader.name() == "pubDate")
                    pubDate = xmlReader.readElementText().left(16);
                
                if (!pubDate.isEmpty()) {
                    auto blogLink = new BlogLinkLabel(BlogLinkTemplate.arg(link, title));
                    blogLink->setMouseTracking(true);
                    blogLink->setAttribute(Qt::WA_Hover);
                    blogLink->installEventFilter(this);
                    blogLink->setTextInteractionFlags(Qt::TextBrowserInteraction);
                    blogLink->setOpenExternalLinks(true);
                    _blogLinksLay->addWidget(blogLink);
                    _blogLinksLay->addWidget(new QLabel("<font color='gray'>" + pubDate + "</font>"));
                    _blogLinksLay->addSpacing(_blogLinksLay->spacing());
                    pubDate.clear();
                    ++count;
                    if (MaxBlogCountShown == count)
                        break;
                }
            }
        }

        // Save into cache
        saveIntoCache(RssFileName, data, Rss_LastModifiedDateKey, "NotImplemented");
    }


    void WelcomeTab::on_allBlogsButton_clicked()
    {
        QDesktopServices::openUrl(QUrl("http://blog.robomongo.org/"));
    }
    
    void WelcomeTab::on_clearButton_clicked()
    {
        // Ask user
        int const answer = QMessageBox::question(this,
            "Clear Recent Connections", "This will delete all recent connections, are you sure?",
            QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

        if (answer != QMessageBox::Yes)
            return;

        // Delete all recent connection entries
        while (auto hlay = dynamic_cast<QHBoxLayout*>(_recentConnsLay->takeAt(0))) {
            while (auto item = hlay->takeAt(0))
                delete item->widget();

            delete hlay;
        }
        
        _clearButton->setDisabled(true);
        _clearButton->setFocus();
        _clearButton->setStyleSheet("");
        _parent->verticalScrollBar()->setValue(0);  

        // Clear recent connections in config. file
        AppRegistry::instance().settingsManager()->clearRecentConnections();
        AppRegistry::instance().settingsManager()->save();
    }

    void WelcomeTab::on_deleteButton_clicked()
    {
        auto button = dynamic_cast<CustomButton*>(sender());
        if (!button)
            return;

        // Remove label and button from UI
        _recentConnsLay->removeItem(button->parentLayout);
        delete button->label;
        delete button;

        // Disable clear all button conditionally
        if (_recentConnsLay->count() == 0) {
            _clearButton->setDisabled(true);
            _clearButton->setFocus();
            _clearButton->setStyleSheet("");
            _parent->verticalScrollBar()->setValue(0);
        }

        // todo: Clear and rebuild recent connections vector and save into config. file
        auto const settingsManager = AppRegistry::instance().settingsManager();
        _recentConnections.clear();
        for (int i = 0; i < _recentConnsLay->count(); ++i) {
            auto hlay = dynamic_cast<QHBoxLayout*>(_recentConnsLay->itemAt(i));
            for (int i = 0; i < hlay->count(); ++i) {
                auto label = dynamic_cast<QLabel*>(hlay->itemAt(i)->widget());
                if (label) {
                    // todo: "href='uuid'>%2</a></p>");
                    auto const uuid = label->text().split("href='")[1].split("'")[0];
                    _recentConnections.push_back(settingsManager->getConnectionSettingsByUuid(uuid));
                }
            }
        }
        AppRegistry::instance().settingsManager()->setRecentConnections(_recentConnections);
        AppRegistry::instance().settingsManager()->save();
    }

    void WelcomeTab::linkActivated(QString const& link)
    {
        for (auto const conn : AppRegistry::instance().settingsManager()->connections()) {
            if (conn->uuid() == link) {
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

        auto const settingsManager = AppRegistry::instance().settingsManager();
        auto conn = settingsManager->getConnectionSettingsByUuid(event->connInfo._uuid);

        // Remove duplicate label and it's button
        for (int i = 0; i < _recentConnsLay->count(); ++i) {
            auto hlay = dynamic_cast<QHBoxLayout*>(_recentConnsLay->itemAt(i));
            for (int i = 0; i < hlay->count(); ++i) {
                auto label = dynamic_cast<QLabel*>(hlay->itemAt(i)->widget());
                if (label) {
                    // todo: "href='uuid'>%2</a></p>");
                    auto const uuid = label->text().split("href='")[1].split("'")[0];
                    if (uuid == conn->uuid()) {
                        while (auto item = hlay->takeAt(0)) // delete label and button
                            delete item->widget();

                        _recentConnsLay->removeItem(hlay);
                    }
                }
            }
        }

        addRecentConnectionItem(conn, true);

        // todo: Clear and rebuild recent connections vector and save into config. file
        _recentConnections.clear();
        for (int i = 0; i < _recentConnsLay->count(); ++i) {
            auto hlay = dynamic_cast<QHBoxLayout*>(_recentConnsLay->itemAt(i));
            for (int i = 0; i < hlay->count(); ++i) {
                auto label = dynamic_cast<QLabel*>(hlay->itemAt(i)->widget());
                if (label) {
                    // todo: "href='uuid'>%2</a></p>");
                    auto const uuid = label->text().split("href='")[1].split("'")[0];
                    _recentConnections.push_back(settingsManager->getConnectionSettingsByUuid(uuid));
                }
            }
        }
        AppRegistry::instance().settingsManager()->setRecentConnections(_recentConnections);
        AppRegistry::instance().settingsManager()->save();
    }

    
    //void WelcomeTab::genericNetworkReplyHandler(QNetworkReply* reply, QString const& fileName, 
    //                                            QString const& lastModifiedKey)
    //{
    //    if (reply->operation() == QNetworkAccessManager::HeadOperation) {
    //        if (reply->error() == QNetworkReply::NoError) { // No network error
    //            QString const& createDate = reply->header(QNetworkRequest::LastModifiedHeader).toString();
    //            if (createDate == AppRegistry::instance().settingsManager()->cacheData(lastModifiedKey)
    //                && fileExists(CacheDir + fileName))
    //            {
    //                // Load from cache
    //                auto file = loadFileFromCache(fileName);
    //                setText1FromCache(file.get());
    //                return;
    //            }
    //            else {  // Get from internet
    //                reply->manager()->get(QNetworkRequest(reply->url()));
    //            }
    //        }
    //        else {  // There is a network error 
    //            // Load from cache
    //            auto file = loadFileFromCache(fileName);
    //            QTextStream in(file.get());
    //            QString str(in.readAll());
    //            _whatsNewText->setText(str);
    //            return;
    //        }
    //    }
    //    else if (reply->operation() == QNetworkAccessManager::GetOperation) {
    //        // todo: handle get operation fails
    //        QString str(QUrl::fromPercentEncoding(reply->readAll()));
    //        if (str.isEmpty()) {
    //            LOG_MSG("WelcomeTab: Failed to download text file from URL. Reason: " + reply->errorString(),
    //                mongo::logger::LogSeverity::Warning());
    //            // todo: load from cache?
    //            return;
    //        }
    //        _whatsNewText->setText(str);
    //        saveIntoCache(text1_URL.fileName(), str, Text1_LastModifiedDate,
    //            reply->header(QNetworkRequest::LastModifiedHeader).toString());
    //    }
    //}


    //void WelcomeTab::setText1FromCache(QFile* cacheFile)
    //{
    //    if (!cacheFile)
    //        return;

    //    QTextStream in(cacheFile);
    //    QString str(in.readAll());
    //    _whatsNewText->setText(str);
    //}

    void WelcomeTab::addRecentConnectionItem(ConnectionSettings const* conn, bool insertTop)
    {
        if (!conn)
            return;

        auto connLabel = new QLabel(connLabelTemplate.arg(conn->uuid(),
            QString::fromStdString(conn->connectionName() + " (" + conn->hostAndPort().toString() + ')')));

        connLabel->setMouseTracking(true);
        connLabel->setAttribute(Qt::WA_Hover);
        connLabel->installEventFilter(this);
        VERIFY(connect(connLabel, SIGNAL(linkActivated(QString)), this, SLOT(linkActivated(QString))));
        VERIFY(connect(connLabel, SIGNAL(linkHovered(QString)), this, SLOT(linkHovered(QString))));

        auto button = new CustomButton;
        std::call_once(onceFlag1, [&]{ CustomButtonHeight = button->iconSize().height()*0.7; });
        button->setStyleSheet("border: none;");
        button->installEventFilter(this);
        VERIFY(connect(button, SIGNAL(clicked()), this, SLOT(on_deleteButton_clicked())));
        
        connLabel->setBuddy(button);
        button->setLabelOnLeft(connLabel);

        auto hlay = new QHBoxLayout;
        hlay->addWidget(connLabel);
        hlay->addWidget(button);
        hlay->setAlignment(Qt::AlignLeft);
        hlay->setSpacing(0);

        button->setParentLayout(hlay);

        if (insertTop)
            _recentConnsLay->insertLayout(0, hlay);
        else
            _recentConnsLay->addLayout(hlay);

        _clearButton->setEnabled(true);
        _clearButton->setStyleSheet("color: #106CD6");
    }

    void WelcomeTab::removeRecentConnectionItem(ConnectionSettings const* conn)
    {
        // todo: to func.
        for (int i = 0; i < _recentConnsLay->count(); ++i) {
            auto hlay = dynamic_cast<QHBoxLayout*>(_recentConnsLay->itemAt(i));
            for (int i = 0; i < hlay->count(); ++i) {
                auto label = dynamic_cast<QLabel*>(hlay->itemAt(i)->widget());
                if (label) {
                    // todo: "href='uuid'>%2</a></p>");
                    auto const uuid = label->text().split("href='")[1].split("'")[0];
                    if (uuid == conn->uuid()) {
                        // Remove label & button and their hlayout from UI
                        while (auto item = hlay->takeAt(0)) // delete label and button
                            delete item->widget();

                        _recentConnsLay->removeItem(hlay);
                    }
                }
            }
        }
    }

    bool WelcomeTab::eventFilter(QObject *target, QEvent *event)
    {
        auto label = qobject_cast<QLabel*>(target);
        auto blogLinkLabel = dynamic_cast<BlogLinkLabel*>(target);
        
        if (label && !blogLinkLabel) {
            auto but = qobject_cast<QPushButton*>(label->buddy());
            if (event->type() == QEvent::HoverEnter) {
                label->setText(label->text().replace("text-decoration: none;", "text-decoration: ;"));
                setCursor(Qt::PointingHandCursor);
                but->setIcon(GuiRegistry::instance().deleteIcon());
                but->setIconSize(QSize(but->iconSize().width(), CustomButtonHeight));
                return true;
            }
            else  if (event->type() == QEvent::HoverLeave) {
                label->setText(label->text().replace("text-decoration: ;", "text-decoration: none;"));
                setCursor(Qt::ArrowCursor);
                but->setIcon(QIcon(""));
                return true;
            }
        }

        if (blogLinkLabel) {
            if (event->type() == QEvent::HoverEnter) {
                blogLinkLabel->setText(blogLinkLabel->text().replace("text-decoration: none;", "text-decoration: ;"));
                setCursor(Qt::PointingHandCursor);
                return true;
            }
            else  if (event->type() == QEvent::HoverLeave) {
                blogLinkLabel->setText(blogLinkLabel->text().replace("text-decoration: ;", "text-decoration: none;"));
                setCursor(Qt::ArrowCursor);
                return true;
            }
        }

        auto but = qobject_cast<QPushButton*>(target);
        if (but) {
            if (event->type() == QEvent::HoverEnter) {
                but->setIcon(GuiRegistry::instance().deleteIconMouseHovered());
                but->setIconSize(QSize(but->iconSize().width(), CustomButtonHeight));
                setCursor(Qt::PointingHandCursor);
                return true;
            }
            else  if (event->type() == QEvent::HoverLeave) {
                but->setIcon(QIcon(""));
                setCursor(Qt::ArrowCursor);
                return true;
            }
        }

        return QWidget::eventFilter(target, event);
    }


}
