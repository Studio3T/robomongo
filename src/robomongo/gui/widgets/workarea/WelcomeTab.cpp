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
#include <QFileInfo>
#include <QDesktopServices>
#include <QScrollArea>
#include <QScrollBar>
#include <QEvent>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/utils/Logger.h"       
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
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

    QUrl const pic1_URL = "http://blog.robomongo.org/content/images/2017/02/bottom.png";

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
        _clearButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        clearButtonLay->addWidget(_clearButton);
        clearButtonLay->addStretch();

        _whatsNewSection = new QLabel(whatsNew + str1 + str2);
        _whatsNewSection->setTextInteractionFlags(Qt::TextSelectableByMouse);
        _whatsNewSection->setWordWrap(true);
        _whatsNewSection->setMinimumWidth(500);
        _whatsNewSection->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        _whatsNewSection->setMinimumHeight(_whatsNewSection->sizeHint().height());

        _pic1 = new QLabel;
        //pic1->setPixmap(pic1URL);
        //_pic1->setMaximumSize(800,400);
        _pic1->setTextInteractionFlags(Qt::TextSelectableByMouse);
        //pic1->setStyleSheet("background-color: gray");

        auto manager = new QNetworkAccessManager;
        VERIFY(connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*))));

        auto const& cacheDir = QString("%1/.config/robomongo/1.0/cache/").arg(QDir::homePath());
        if (QDir(cacheDir).exists()) {
            QFileInfo check_file(cacheDir + pic1_URL.fileName());
            if (check_file.exists() && check_file.isFile()) {   // Use cached file
                QPixmap img(cacheDir + pic1_URL.fileName());
                _pic1->setPixmap(img.scaledToWidth(_whatsNewSection->width()));
            }
            else {  // Get file from Internet
                manager->get(QNetworkRequest(pic1_URL));
            }
        }

        auto blogsSection = new QLabel(blogPosts + blog0 + blog1 + blog2 + blog3 + blog4);
        blogsSection->setTextInteractionFlags(Qt::TextSelectableByMouse);
        blogsSection->setWordWrap(true);
        blogsSection->setTextFormat(Qt::RichText);
        blogsSection->setTextInteractionFlags(Qt::TextBrowserInteraction);
        blogsSection->setOpenExternalLinks(true);

        auto buttonLay = new QHBoxLayout;
        _allBlogsButton = new QPushButton("All Blog Posts");
        //button->setStyleSheet("font-size: 14pt; background-color: green; border: 2px; color: white");        
        _allBlogsButton->setStyleSheet("color: #106CD6");
        VERIFY(connect(_allBlogsButton, SIGNAL(clicked()), this, SLOT(on_allBlogsButton_clicked())));
        _allBlogsButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        buttonLay->addWidget(_allBlogsButton);
        buttonLay->addStretch();

        auto rightLayout = new QVBoxLayout;
        rightLayout->setContentsMargins(20, -1, -1, -1);
        rightLayout->addWidget(blogsSection, 0, Qt::AlignTop);
        rightLayout->addSpacing(15);
        rightLayout->addLayout(buttonLay);
        rightLayout->addStretch();

        auto leftLayout = new QVBoxLayout;
        leftLayout->addWidget(recentConnLabel, 0, Qt::AlignTop);
        leftLayout->addLayout(_recentConnsLay);
        leftLayout->addSpacing(10);
        leftLayout->addLayout(clearButtonLay);
        leftLayout->addStretch();
        leftLayout->addSpacing(30);
        leftLayout->addWidget(_whatsNewSection, 0, Qt::AlignTop);
        leftLayout->addWidget(_pic1, 0, Qt::AlignTop);

        auto mainLayout = new QHBoxLayout;
        mainLayout->setContentsMargins(20, -1, -1, -1);
        mainLayout->addLayout(leftLayout);
        mainLayout->addLayout(rightLayout);
        //mainLayout->addStretch();
        mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        setLayout(mainLayout);

        //setStyleSheet("background-color: yellow");
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

        _pic1->setPixmap(img->scaledToWidth(_whatsNewSection->width()));

        // Save to cache
        auto const& cacheDir = QString("%1/.config/robomongo/1.0/cache/").arg(QDir::homePath());
        if (!QDir(cacheDir).exists())
            QDir().mkdir(cacheDir);
        else {  // cache dir. exists
            QFileInfo check_file(cacheDir + pic1_URL.fileName());
            // Make sure we delete the old file in order to cache the newly downloaded file
            if (check_file.exists() && check_file.isFile()) {
                if (!QFile::remove(cacheDir + pic1_URL.fileName())) {
                    LOG_MSG("WelcomeTab: Failed to delete cached file at: " + cacheDir + pic1_URL.fileName(),
                             mongo::logger::LogSeverity::Warning());
                }
            }
        }
        img->save(cacheDir + reply->url().fileName());
    }

    void WelcomeTab::on_allBlogsButton_clicked()
    {
        QDesktopServices::openUrl(QUrl("http://blog.robomongo.org/"));
    }

    
    void WelcomeTab::on_clearButton_clicked()
    {
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

    void WelcomeTab::addRecentConnectionItem(ConnectionSettings const* conn, bool insertTop)
    {
        if (!conn)
            return;

        auto connLabel = new QLabel(connLabelTemplate.arg(conn->uuid(),
            QString::fromStdString(conn->connectionName() + " (" + conn->hostAndPort().toString() + ')')));

        connLabel->setMouseTracking(true);
        connLabel->setAttribute(Qt::WA_Hover);
        connLabel->installEventFilter(this);
        connLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
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
        if (label) {
            auto but = qobject_cast<QPushButton*>(label->buddy());
            if (event->type() == QEvent::HoverEnter) {
                label->setText(label->text().replace("text-decoration: none;", "text-decoration: ;"));
                setCursor(Qt::PointingHandCursor);
                but->setIcon(GuiRegistry::instance().deleteIcon());
                but->setIconSize(QSize(but->iconSize().width(), CustomButtonHeight ));
                return true;
            }
            else  if (event->type() == QEvent::HoverLeave) {
                label->setText(label->text().replace("text-decoration: ;", "text-decoration: none;"));
                setCursor(Qt::ArrowCursor);
                but->setIcon(QIcon(""));
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
