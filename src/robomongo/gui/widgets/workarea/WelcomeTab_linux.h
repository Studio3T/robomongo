#pragma once

#include <QWidget>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QPushButton;
class QNetworkReply;
class QLabel;
class QVBoxLayout;
class QScrollArea;
class QEvent;
class QUrl;
QT_END_NAMESPACE

namespace Robomongo
{
    struct ConnectionEstablishedEvent;
    class ConnectionSettings;

    class WelcomeTab : public QWidget
    {
        Q_OBJECT

    public:
        WelcomeTab(QScrollArea *parent = nullptr);
        ~WelcomeTab();
        // Temporarily disabling Recent Connections feature
        // void removeRecentConnectionItem(ConnectionSettings const* conn);
        QScrollArea* getParent() const { return _parent; }
        void resize();

    protected:
        bool eventFilter(QObject *target, QEvent *event) override;

    private Q_SLOTS:
        void on_downloadTextReply(QNetworkReply* reply);
        void on_downloadPictureReply(QNetworkReply* reply);
        void on_downloadRssReply(QNetworkReply* reply);
        void on_allBlogsButton_clicked();
        /* Temporarily disabling Recent Connections feature
        void on_clearButton_clicked();
        void on_deleteButton_clicked();
        void on_recentConnectionLinkClicked(QString const& link);
        void handle(ConnectionEstablishedEvent *event);
        */

    private:
        // Temporarily disabling Recent Connections feature
        // void addRecentConnectionItem(ConnectionSettings const* conn, bool insertTop);

        void setWhatsNewHeaderAndText(QString const& str);

        QLabel* _pic1 = nullptr;
        QLabel* _blogsSection;
        QLabel* _blogsHeader;
        QPushButton* _allBlogsButton = nullptr;
        QVBoxLayout* _blogLinksLay;
        // Temporarily disabling Recent Connections feature
        // QVBoxLayout* _recentConnsLay;
        QLabel* _whatsNewHeader;
        QLabel* _whatsNewText;
        QPushButton* _clearButton;
        QScrollArea* _parent;
        QPixmap _image;

        QUrl _pic1_URL;
        QUrl _text1_URL;
        QUrl _rss_URL;

        // Temporarily disabling Recent Connections feature
        // std::vector<ConnectionSettings const*> _recentConnections;
    };

}
