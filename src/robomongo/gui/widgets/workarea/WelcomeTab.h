#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
class QNetworkReply;
class QLabel;
class QVBoxLayout;
class QScrollArea;
class QEvent;
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
        void removeRecentConnectionItem(ConnectionSettings const* conn);
        QScrollArea* getParent() const { return _parent; }

    protected:
        bool eventFilter(QObject *target, QEvent *event) override;

    private Q_SLOTS:
        void on_downloadTextReply(QNetworkReply* reply);
        void on_downloadPictureReply(QNetworkReply* reply);
        void on_downloadRssReply(QNetworkReply* reply);
        void on_allBlogsButton_clicked();
        void on_clearButton_clicked();
        void on_deleteButton_clicked();
        void linkActivated(QString const& link);
        void linkHovered(QString const& link);
        void handle(ConnectionEstablishedEvent *event);

    private:
        //void genericNetworkReplyHandler();
        //void setText1FromCache(QFile* cacheFile);
        void addRecentConnectionItem(ConnectionSettings const* conn, bool insertTop);

        QLabel* _pic1 = nullptr;
        QLabel *_blogsSection;
        QPushButton* _allBlogsButton = nullptr;
        QVBoxLayout* _blogLinksLay;
        QVBoxLayout* _recentConnsLay;
        QLabel* _whatsNewHeader;
        QLabel* _whatsNewText;
        QPushButton* _clearButton;
        QScrollArea* _parent;

        std::vector<ConnectionSettings const*> _recentConnections;
        int _lastAddedConnId = -1;
    };

}
