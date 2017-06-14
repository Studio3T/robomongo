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
#include <QToolButton>
#include <QMenu>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/MainWindow.h"
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

    /* Temporarily disabling Recent Connections feature
    // todo: brief: Special button to use on the right side of a recent connection label
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
    */
    
    QString const WhatsNew = "<p><h1><font color=\"#2d862d\">%1</h1></p>";
    QString const BlogsHeader = "<p><h1><font color=\"#2d862d\">Blog Posts</h1></p>";
    QString const BlogLinkTemplate = "<a style = 'color: #106CD6; text-decoration: none;'"
                                     "href='%1'>%2</a>";

    // URL Folder number for Pic1 and Text1
    enum {
        URL_FOLDER_1_0_0      = 1,
        URL_FOLDER_1_1_0_BETA = 2,
        URL_FOLDER_1_1_0      = 3
    };

    auto const CURRENT_URL_FOLDER = URL_FOLDER_1_1_0;

// Not using https for Linux due to crashes and unstable behaviors experienced.
#ifdef __linux__
    QUrl const Pic1_URL = QString("https://rm-feed.3t.io/" + QString::number(CURRENT_URL_FOLDER) + "/image.png");
    QUrl const Text1_URL = QString("https://rm-feed.3t.io/" + QString::number(CURRENT_URL_FOLDER) + "/contents.txt");
    QUrl const Rss_URL = QString("http://blog.robomongo.org/rss/");
#else
    QUrl const Pic1_URL = QString("https://rm-feed.3t.io/" + QString::number(CURRENT_URL_FOLDER) + "/image.png");
    QUrl const Text1_URL = QString("https://rm-feed.3t.io/" + QString::number(CURRENT_URL_FOLDER) + "/contents.txt");
    QUrl const Rss_URL = QString("https://blog.robomongo.org/rss/");
#endif

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
        /* Temporarily disabling Recent Connections feature
        AppRegistry::instance().bus()->subscribe(this, ConnectionEstablishedEvent::Type);

       _recentConnsLay = new QVBoxLayout;       
        auto recentConnHeader = new QLabel(Recent_Connections_Header);
        //recentConnHeader->setMinimumWidth(_parent->width()*0.6);

        auto _connectButton = new QToolButton;
        _connectButton->setText("Connect");
        _connectButton->setIcon(GuiRegistry::instance().connectIcon().pixmap(16, 16));
        _connectButton->setFocusPolicy(Qt::NoFocus);
        _connectButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        MainWindow* mainWindow;
        for (auto wid : QApplication::topLevelWidgets()) {
            if (qobject_cast<MainWindow*>(wid)) {
                mainWindow = qobject_cast<MainWindow*>(wid);
                break;
            }
        }
        VERIFY(connect(_connectButton, SIGNAL(clicked()), mainWindow, SLOT(manageConnections())));

        auto connectButtonMenu = new QMenu;
        auto action = new QAction("Clear Recent Connections", this);
        action->setIcon(GuiRegistry::instance().deleteIconRed().pixmap(14, 14));

        VERIFY(connect(action, SIGNAL(triggered()), this, SLOT(on_clearButton_clicked())));
        connectButtonMenu->addAction(action);

#if !defined(Q_OS_MAC)
        _connectButton->setMenu(connectButtonMenu);
        _connectButton->setPopupMode(QToolButton::MenuButtonPopup);
#endif

        // Load and add recent connections from settings
        auto const settingsManager = AppRegistry::instance().settingsManager();
        auto const& recentConnections = settingsManager->recentConnections();
        if (recentConnections.size() < 1) 
            action->setDisabled(true);
        else {
            for (auto const& rconn : recentConnections) 
                // check nullptr for settingsManager->getConnectionSettingsByUuid()
                addRecentConnectionItem(settingsManager->getConnectionSettingsByUuid(rconn.uuid), false);
        }

        auto connectButtonLay = new QHBoxLayout;
        connectButtonLay->addWidget(_connectButton);
        connectButtonLay->addStretch();
        */

        //// What's new section
        _whatsNewHeader = new QLabel;
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
        _pic1->setScaledContents(true);

        _blogsHeader = new QLabel(BlogsHeader);
        _blogsHeader->setHidden(true);

        //// --- Network Access Managers
        auto text1Downloader = new QNetworkAccessManager;
        VERIFY(connect(text1Downloader, SIGNAL(finished(QNetworkReply*)), 
                       this, SLOT(on_downloadTextReply(QNetworkReply*))));
        text1Downloader->head(QNetworkRequest(Text1_URL));

        auto pic1Downloader = new QNetworkAccessManager;
        VERIFY(connect(pic1Downloader, SIGNAL(finished(QNetworkReply*)), 
               this, SLOT(on_downloadPictureReply(QNetworkReply*))));
        pic1Downloader->head(QNetworkRequest(Pic1_URL));

        auto rssDownloader = new QNetworkAccessManager;
        VERIFY(connect(rssDownloader, SIGNAL(finished(QNetworkReply*)), 
                       this, SLOT(on_downloadRssReply(QNetworkReply*))));
        rssDownloader->get(QNetworkRequest(Rss_URL));

        //// --- Layouts
        _allBlogsButton = new QPushButton("All Blog Posts");
        _allBlogsButton->setHidden(true);
        _allBlogsButton->setStyleSheet("color: #106CD6");
        VERIFY(connect(_allBlogsButton, SIGNAL(clicked()), this, SLOT(on_allBlogsButton_clicked())));
        _allBlogsButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);


        _blogLinksLay = new QVBoxLayout;
        _blogLinksLay->setAlignment(Qt::AlignLeft);

        auto rightLayout = new QVBoxLayout;
        rightLayout->setContentsMargins(20, -1, -1, -1);
        rightLayout->addWidget(_blogsHeader, 0, Qt::AlignTop);
        rightLayout->addLayout(_blogLinksLay);
        rightLayout->addSpacing(15);
        rightLayout->addWidget(_allBlogsButton, 0, Qt::AlignLeft);
        rightLayout->addStretch();

        auto leftLayout = new QVBoxLayout;
        /* Temporarily disabling Recent Connections feature
        leftLayout->addWidget(recentConnHeader, 0, Qt::AlignTop);
        leftLayout->addLayout(_recentConnsLay);
        leftLayout->addSpacing(10);
        leftLayout->addLayout(connectButtonLay);
        //leftLayout->addStretch();
        leftLayout->addSpacing(20);
        */
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
        mainLayout->setSizeConstraint(QLayout::SetMinimumSize);

        setLayout(mainLayout);
    }

    WelcomeTab::~WelcomeTab()
    {

    }

    void WelcomeTab::on_downloadTextReply(QNetworkReply* reply)
    {
        auto hideOrShowWhatsNewHeader = [this]() {
            if ((!_pic1->pixmap() || _pic1->pixmap()->isNull()) && _whatsNewText->text().isEmpty())
                _whatsNewHeader->setHidden(true);
            else 
                _whatsNewHeader->setVisible(true);
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
                    setWhatsNewHeaderAndText(str);
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
                setWhatsNewHeaderAndText(str);
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
                hideOrShowWhatsNewHeader();
                return;
            }

            setWhatsNewHeaderAndText(str);
            hideOrShowWhatsNewHeader();
            saveIntoCache(Text1_URL.fileName(), str, Text1_LastModifiedDateKey,
                          reply->header(QNetworkRequest::LastModifiedHeader).toString());
        }
    }

    void WelcomeTab::on_downloadPictureReply(QNetworkReply* reply)
    {
        auto const FIFTY_PERCENT_OF_TAB = _parent->width() * IMAGE_TO_TAB_RATIO;

        QPixmap image;
        auto hideOrShowWhatsNewHeader = [this]() {
            if ((!_pic1->pixmap() || _pic1->pixmap()->isNull()) && _whatsNewText->text().isEmpty())
                _whatsNewHeader->setHidden(true);
            else
                _whatsNewHeader->setVisible(true);
        };

        // Network error, load from cache
        if (reply->error() != QNetworkReply::NoError) { 
            image = QPixmap(CacheDir + Pic1_URL.fileName());
            if (image.isNull())
                return;

            _image = image;
            _pic1->setPixmap(_image);

            if (0 == _image.size().width())
                return;

            _pic1->setFixedSize(FIFTY_PERCENT_OF_TAB, (FIFTY_PERCENT_OF_TAB / _image.size().width()) * _image.size().height());
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

        if (0 == _image.size().width()) 
            return

        hideOrShowWhatsNewHeader();
        resize();
    }

    void WelcomeTab::on_downloadRssReply(QNetworkReply* reply)
    {
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

        _blogsHeader->setVisible(true);
        _allBlogsButton->setVisible(true);
        adjustSize();

        // Save into cache
        saveIntoCache(RssFileName, data, Rss_LastModifiedDateKey, "NotImplemented");    // todo
    }

    void WelcomeTab::on_allBlogsButton_clicked()
    {
        QDesktopServices::openUrl(QUrl("https://blog.robomongo.org/"));
    }

    void WelcomeTab::setWhatsNewHeaderAndText(QString const& str)
    {
        if (!str.contains("\n") || str.size() == 0)
            return;

        auto const firstNewLineIndex = str.indexOf("\n");
        auto const leftOfStr = str.left(firstNewLineIndex);
        auto const rightOfStr = str.right(str.size() - firstNewLineIndex - 7);
        _whatsNewHeader->setText(WhatsNew.arg(leftOfStr));
        _whatsNewText->setText(rightOfStr);
        auto const SIXTY_PERCENT_OF_TAB = _parent->width() * TEXT_TO_TAB_RATIO;
        _whatsNewText->setMinimumWidth(SIXTY_PERCENT_OF_TAB);
        adjustSize();
    }
    
/* Temporarily disabling Recent Connections feature
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
        
        // todo: disable clear all action
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

        // todo: Refactor for clear all action
        //// Disable "clear all" button conditionally
        //if (_recentConnsLay->count() == 0) {
        //    _clearButton->setDisabled(true);
        //    _clearButton->setFocus();
        //    _clearButton->setStyleSheet("");
        //    _parent->verticalScrollBar()->setValue(0);
        //}

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
                    // check nullptr for settingsManager->getConnectionSettingsByUuid()
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
        // check nullptr for settingsManager->getConnectionSettingsByUuid()
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
                    // check nullptr for settingsManager->getConnectionSettingsByUuid()
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

        // todo: refactor for clear all action
        //_clearButton->setEnabled(true);
        //_clearButton->setStyleSheet("color: #106CD6");
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
*/

    void WelcomeTab::resize()
    {
        auto const tabWidth = _parent->width();
        auto const FIFTY_PERCENT_OF_TAB = _parent->width() * IMAGE_TO_TAB_RATIO;

        _whatsNewText->setFixedWidth(tabWidth * TEXT_TO_TAB_RATIO);
        _pic1->setPixmap(_image);

        if (0 == _image.size().width())
            return;

        _pic1->setFixedSize(FIFTY_PERCENT_OF_TAB, (FIFTY_PERCENT_OF_TAB / _image.size().width())*_image.size().height());

        _blogsHeader->setFixedWidth(tabWidth * BLOG_TO_TAB_RATIO);
        for (int i = 0; i < _blogLinksLay->count(); ++i) {
            if(auto wid = _blogLinksLay->itemAt(i)->widget())
                wid->setFixedWidth(tabWidth * BLOG_TO_TAB_RATIO);
        }

        adjustSize();
    }

    bool WelcomeTab::eventFilter(QObject *target, QEvent *event)
    {
        /* Temporarily disabling Recent Connections feature
        auto label = qobject_cast<QLabel*>(target);     // todo: recentConnLabel
        */
        auto blogLinkLabel = dynamic_cast<BlogLinkLabel*>(target);
        
        /* Temporarily disabling Recent Connections feature
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
        */

        // Make blog link underlined on mouse hover
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

        /* Temporarily disabling Recent Connections feature
        // todo: refactor to delete recent conn button
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
        */

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

}
