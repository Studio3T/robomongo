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
    bool deleteOldCacheFile(QString const& absFilePath);

    bool saveIntoCache(QString const& fileName, QString const& fileData,
                       QString const& lastModifiedKey, QString const& lastModifiedDate);

    bool saveIntoCache(QString const& fileName, QPixmap const& pixMap,
                       QString const& lastModifiedKey, QString const& lastModifiedDate);

    bool saveIntoCache(QString const& fileName, QByteArray* data,
                       QString const& lastModifiedKey, QString const& lastModifiedDate);

    /**
    * @brief Container structure to hold data of a blog link
    */
    struct BlogInfo
    {
        BlogInfo(QString const& title, QString const& link, QString const& publishDate)
            : title(title), link(link), publishDate(publishDate) {}

        QString const title;
        QString const link;
        QString const publishDate;
    };

    /**
    * @brief Custom label for identifying a blog link label
    */
    struct BlogLinkLabel : public QLabel
    {
        BlogLinkLabel(QString const& args) 
            : QLabel(args) {}
    };

    /* todo: brief: Special button to use on the right side of a recent connection label */
    struct CustomDeleteButton : public QPushButton
    {
        // todo: use ctor. instead of two funcs.
        void setLabelOnLeft(QLabel* lab) { label = lab; }
        void setParentLayout(QLayout* lay) { parentLayout = lay; }

        QLabel* label = nullptr;
        QLayout* parentLayout = nullptr;
    };

    // todo: brief
    std::once_flag onceFlag1;
    int CustomButtonHeight = -1;

    QString const Recent_Connections_Header = "<p><h1><font color=\"#2d862d\">Recent Connections</h1></p>";

    QString const connLabelTemplate = "<p><a style='font-size:14px; color: #106CD6; text-decoration: none;'"
                                      "href='%1'>%2</a></p>";
    
    QString const whatsNew = "<p><h1><font color=\"#2d862d\">What's New</h1></p>";

    QString const BlogsHeader = "<p><h1><font color=\"#2d862d\">Blog Posts</h1></p>";

    QString const BlogLinkTemplate = "<a style = 'font-size:14px; color: #106CD6; text-decoration: none;'"
                                     "href='%1'>%2</a>";

    QUrl const Pic1_URL = QString("https://rm-feed.3t.io/1/image.png");
    QUrl const Text1_URL = QString("https://rm-feed.3t.io/1/contents.txt");
    QString const RssFileName = "rss.xml";

    QString const Text1_LastModifiedDateKey("wtText1LastModifiedDate");
    QString const Image1_LastModifiedDateKey("wtImage1LastModifiedDate");
    QString const Rss_LastModifiedDateKey("wtRssLastModifiedDate");

    auto const TEXT_TO_TAB_RATIO = 0.6; 
    auto const IMAGE_TO_TAB_RATIO = 0.5;
    auto const BLOG_TO_TAB_RATIO = 0.28;

/* ------------------------------------- Welcome Tab --------------------------------------- */

    WelcomeTab::WelcomeTab(QScrollArea *parent) :
        QWidget(parent), _parent(parent)
    {
        AppRegistry::instance().bus()->subscribe(this, ConnectionEstablishedEvent::Type);

        _recentConnsLay = new QVBoxLayout;       
        auto recentConnHeader = new QLabel(Recent_Connections_Header);
        //recentConnHeader->setMinimumWidth(_parent->width()*0.6);

        auto clearButtonLay = new QHBoxLayout;
        _clearButton = new QPushButton("Clear Recent Connections");

        // Load and add recent connections from settings
        auto const settingsManager = AppRegistry::instance().settingsManager();
        auto const& recentConnections = settingsManager->recentConnections();
        if (recentConnections.size() < 1) 
            _clearButton->setDisabled(true);
        else {
            _clearButton->setStyleSheet("color: #106CD6");
            for (auto const& rconn : recentConnections) 
                addRecentConnectionItem(settingsManager->getConnectionSettingsByUuid(rconn.uuid), false);
        }

        VERIFY(connect(_clearButton, SIGNAL(clicked()), this, SLOT(on_clearButton_clicked())));
        //_clearButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        clearButtonLay->addWidget(_clearButton);
        clearButtonLay->addStretch();

        //// What's new section
        _whatsNewHeader = new QLabel(whatsNew);
        _whatsNewHeader->setHidden(true);
        _whatsNewText = new QLabel;
        _whatsNewText->setTextInteractionFlags(Qt::TextSelectableByMouse);
        _whatsNewText->setTextFormat(Qt::RichText);
        _whatsNewText->setTextInteractionFlags(Qt::TextBrowserInteraction);
        _whatsNewText->setOpenExternalLinks(true);
        _whatsNewText->setWordWrap(true);
        _whatsNewText->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

        _pic1 = new QLabel;
        _pic1->setTextInteractionFlags(Qt::TextSelectableByMouse);
        _pic1->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

        _blogsHeader = new QLabel(BlogsHeader);
        //_blogsHeader->setStyleSheet("background-color: yellow;");     

        //// --- Network Access Managers
        auto text1Downloader = new QNetworkAccessManager;
        VERIFY(connect(text1Downloader, SIGNAL(finished(QNetworkReply*)), this, SLOT(on_downloadTextReply(QNetworkReply*))));
        text1Downloader->head(QNetworkRequest(Text1_URL));

        auto pic1Downloader = new QNetworkAccessManager;
        VERIFY(connect(pic1Downloader, SIGNAL(finished(QNetworkReply*)), 
               this, SLOT(on_downloadPictureReply(QNetworkReply*))));
        pic1Downloader->head(QNetworkRequest(Pic1_URL));

        auto rssDownloader = new QNetworkAccessManager;
        VERIFY(connect(rssDownloader, SIGNAL(finished(QNetworkReply*)), this, SLOT(on_downloadRssReply(QNetworkReply*))));
        QUrl const rssURL = QString("https://blog.robomongo.org/rss/");
        rssDownloader->get(QNetworkRequest(rssURL));

        //// --- Layouts
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
        rightLayout->addWidget(_blogsHeader, 0, Qt::AlignTop);
        rightLayout->addLayout(_blogLinksLay);
        rightLayout->addSpacing(15);
        rightLayout->addLayout(buttonLay);
        rightLayout->addStretch();

        auto leftLayout = new QVBoxLayout;
        //leftLayout->setSpacing(0);
        leftLayout->addWidget(recentConnHeader, 0, Qt::AlignTop);
        leftLayout->addLayout(_recentConnsLay);
        leftLayout->addSpacing(10);
        leftLayout->addLayout(clearButtonLay);
        //leftLayout->addStretch();
        leftLayout->addSpacing(20);
        leftLayout->addWidget(_whatsNewHeader, 0, Qt::AlignTop);
        leftLayout->addWidget(_pic1, 0, Qt::AlignTop);
        leftLayout->addWidget(_whatsNewText, 0, Qt::AlignTop);
        leftLayout->addStretch();
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
        auto const SIXTY_PERCENT_OF_TAB = _parent->width() * TEXT_TO_TAB_RATIO;

        auto hideOrShowWhatsNewHeader = [this]() {
            _whatsNewHeader->setHidden( (!_pic1->pixmap() || _pic1->pixmap()->isNull()) 
                                        && _whatsNewText->text().isEmpty() ? true : false);
        };

        if (reply->operation() == QNetworkAccessManager::HeadOperation) {
            if (reply->error() == QNetworkReply::NoError) { // No network error
                QString const& createDate = reply->header(QNetworkRequest::LastModifiedHeader).toString();
                if (createDate == AppRegistry::instance().settingsManager()->cacheData(Text1_LastModifiedDateKey)
                    && fileExists(CacheDir + Text1_URL.fileName())) 
                {
                    // Load from cache
                    QFile file(CacheDir + Text1_URL.fileName());
                    if (!file.open(QFile::ReadOnly | QFile::Text))
                        return;

                    QTextStream in(&file);
                    QString str(in.readAll());
                    _whatsNewText->setText(str);
                    _whatsNewText->setMinimumWidth(SIXTY_PERCENT_OF_TAB);
                    adjustSize();
                    hideOrShowWhatsNewHeader();
                    return;
                }
                else {  // Get from internet
                    reply->manager()->get(QNetworkRequest(Text1_URL));
                }
            }
            else {  // There is a network error 
                // Load from cache
                QFile file(CacheDir + Text1_URL.fileName());
                if (!file.open(QFile::ReadOnly | QFile::Text))
                    return;

                QTextStream in(&file);
                QString str(in.readAll());
                _whatsNewText->setText(str);
                _whatsNewText->setMinimumWidth(SIXTY_PERCENT_OF_TAB);
                adjustSize();
                hideOrShowWhatsNewHeader();
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
                hideOrShowWhatsNewHeader();
                return;
            }
            _whatsNewText->setText(str);
            _whatsNewText->setMinimumWidth(SIXTY_PERCENT_OF_TAB);
            adjustSize();
            saveIntoCache(Text1_URL.fileName(), str, Text1_LastModifiedDateKey,
                          reply->header(QNetworkRequest::LastModifiedHeader).toString());
        }
    }

    void WelcomeTab::on_downloadPictureReply(QNetworkReply* reply)
    {
        auto const FIFTY_PERCENT_OF_TAB = _parent->width() * IMAGE_TO_TAB_RATIO;

        QPixmap image;
        auto hideOrShowWhatsNewHeader = [this]() {
            _whatsNewHeader->setHidden( (!_pic1->pixmap() || _pic1->pixmap()->isNull()) 
                                        && _whatsNewText->text().isEmpty() ? true : false);        
        };

        if (reply->error() != QNetworkReply::NoError) { // Network error, load from cache
            image = QPixmap(CacheDir + Pic1_URL.fileName());
            if (image.isNull())
                return;

            _pic1->setPixmap(image.scaledToWidth(FIFTY_PERCENT_OF_TAB));
            adjustSize();
            hideOrShowWhatsNewHeader();
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
                    hideOrShowWhatsNewHeader();
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
        if (image.isNull())
            return;

        _image = image;
        _pic1->setPixmap(_image.scaledToWidth(FIFTY_PERCENT_OF_TAB));
        adjustSize();
        hideOrShowWhatsNewHeader();
    }

    void WelcomeTab::on_downloadRssReply(QNetworkReply* reply)
    {
        // todo: if reply->error(), log and return
        // todo: load from cache if download fails

        auto const THIRTY_PERCENT_OF_TAB = _parent->width() * BLOG_TO_TAB_RATIO;

        QByteArray data = reply->readAll();
        if (data.isEmpty() || reply->error() != QNetworkReply::NoError) {
            // Load from cache
            QFile file(CacheDir + RssFileName);
            if (!file.open(QFile::ReadOnly | QFile::Text))
                return;

            data = file.readAll();
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
                    blogLink->setWordWrap(true);
                    blogLink->setMinimumWidth(THIRTY_PERCENT_OF_TAB);
                    //blogLink->setStyleSheet("background-color: yellow;");
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
        adjustSize();

        // Save into cache
        saveIntoCache(RssFileName, data, Rss_LastModifiedDateKey, "NotImplemented");    // todo
    }

    void WelcomeTab::on_allBlogsButton_clicked()
    {
        QDesktopServices::openUrl(QUrl("https://blog.robomongo.org/"));
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
        //// Ask user
        //int const answer = QMessageBox::question(this,
        //    "Delete Recent Connection", "This will delete this recent connection link, are you sure?",
        //    QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

        //if (answer != QMessageBox::Yes)
        //    return;

        auto button = dynamic_cast<CustomDeleteButton*>(sender());
        if (!button)
            return;

        // Remove label and button from UI
        _recentConnsLay->removeItem(button->parentLayout);
        delete button->label;
        delete button;

        // Disable "clear all" button conditionally
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
        settingsManager->setRecentConnections(_recentConnections);
        settingsManager->save();
    }

    void WelcomeTab::on_recentConnectionLinkClicked(QString const& link)
    {
        for (auto const conn : AppRegistry::instance().settingsManager()->connections()) {
            if (conn->uuid() == link) {
                AppRegistry::instance().app()->openServer(conn, ConnectionPrimary);
                return;
            }
        }
    }

    void WelcomeTab::handle(ConnectionEstablishedEvent *event)
    {
        // Do not make UI changes for non PRIMARY connections
        if (event->connectionType != ConnectionPrimary)
            return;

        auto const settingsManager = AppRegistry::instance().settingsManager();
        auto conn = settingsManager->getConnectionSettingsByUuid(event->connInfo._uuid);

        if (!conn)
            return;

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
        settingsManager->setRecentConnections(_recentConnections);
        settingsManager->save();
    }

    void WelcomeTab::addRecentConnectionItem(ConnectionSettings const* conn, bool insertTop)
    {
        if (!conn)
            return;

        auto connLabel = new QLabel(connLabelTemplate.arg(conn->uuid(),
            QString::fromStdString(conn->connectionName() + " (" + conn->hostAndPort().toString() + ')')));

        connLabel->setMouseTracking(true);
        connLabel->setAttribute(Qt::WA_Hover);
        connLabel->installEventFilter(this);
        VERIFY(connect(connLabel, SIGNAL(linkActivated(QString)), this, SLOT(on_recentConnectionLinkClicked(QString))));

        auto button = new CustomDeleteButton;
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

    void WelcomeTab::resize()
    {
        auto const tabWidth = _parent->width();

        _whatsNewText->setFixedWidth(tabWidth * TEXT_TO_TAB_RATIO);
        _pic1->setPixmap(_image.scaledToWidth(tabWidth * IMAGE_TO_TAB_RATIO));

        _blogsHeader->setFixedWidth(tabWidth * BLOG_TO_TAB_RATIO);
        for (int i = 0; i < _blogLinksLay->count(); ++i) {
            if(auto wid = _blogLinksLay->itemAt(i)->widget())
                wid->setFixedWidth(tabWidth * BLOG_TO_TAB_RATIO);
        }

        adjustSize();
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

    bool deleteOldCacheFile(QString const& absFilePath)
    {
        if (!fileExists(absFilePath))
            return true;

        if (!QFile::remove(absFilePath)) {
            LOG_MSG("WelcomeTab: Failed to delete cached file at: " + absFilePath,
                     mongo::logger::LogSeverity::Warning());
            return false;
        }

        return true;
    }

    bool saveIntoCache(QString const& fileName, QString const& fileData,
                       QString const& lastModifiedKey, QString const& lastModifiedDate)
    {
        if (!QDir(CacheDir).exists())
            QDir().mkdir(CacheDir);
        else 
            deleteOldCacheFile(CacheDir + fileName);

        QFile file(CacheDir + fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return false;

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
        if (!QDir(CacheDir).exists())
            QDir().mkdir(CacheDir);
        else 
            deleteOldCacheFile(CacheDir + fileName);

        QFile file(CacheDir + fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return false;

        pixMap.save(CacheDir + fileName);

        // Save file's last modified date into settings
        AppRegistry::instance().settingsManager()->addCacheData(lastModifiedKey, lastModifiedDate);
        AppRegistry::instance().settingsManager()->save();
        return true;
    }

    bool saveIntoCache(QString const& fileName, QByteArray* data,
                       QString const& lastModifiedKey, QString const& lastModifiedDate)
    {
        if (!QDir(CacheDir).exists())
            QDir().mkdir(CacheDir);
        else 
            deleteOldCacheFile(CacheDir + fileName);

        QFile file(CacheDir + fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return false;

        file.write(*data);

        // Save file's last modified date into settings
        AppRegistry::instance().settingsManager()->addCacheData(lastModifiedKey, lastModifiedDate);
        AppRegistry::instance().settingsManager()->save();
        return true;
    }


    // todo: remove
    //    std::unique_ptr<QFile> loadFileFromCache(QString const& fileName)
    //    {
    ////        if (fileExists(CacheDir + fileName)) {   // Use cached file
    //            std::unique_ptr<QFile> file(new QFile(CacheDir + fileName));
    //            if (file->open(QFile::ReadOnly | QFile::Text)) {
    //                return file;
    //            }
    ////        }
    //
    //        return nullptr;
    //    }

}
