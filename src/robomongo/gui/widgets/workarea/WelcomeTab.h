#pragma once

#ifndef __linux__  // ---------------------- Windows, macOS impl. --------------------------// 

#include <QWidget>

QT_BEGIN_NAMESPACE
class QScrollArea;
QT_END_NAMESPACE

namespace Robomongo
{
    class WelcomeTab : public QWidget
    {
        Q_OBJECT

    public:
        WelcomeTab(QScrollArea *parent = nullptr);
        ~WelcomeTab() {}

        QScrollArea* getParent() const { return _parent; }
        void resize() { /* Not implemented for Windows and macOS */}

    private:
        QScrollArea* _parent;
    };
}

#else   // -------------------------------- Linux impl. ------------------------------------// 

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
        QScrollArea* getParent() const { return _parent; }
        void resize();

    protected:
        bool eventFilter(QObject *target, QEvent *event) override;

    private Q_SLOTS:
        void on_downloadTextReply(QNetworkReply* reply);
        void on_downloadPictureReply(QNetworkReply* reply);
        void on_downloadRssReply(QNetworkReply* reply);
        void on_allBlogsButton_clicked();

    private:
        void setWhatsNewHeaderAndText(QString const& str);

        QLabel* _pic1 = nullptr;
        QLabel* _blogsSection;
        QLabel* _blogsHeader;
        QPushButton* _allBlogsButton = nullptr;
        QVBoxLayout* _blogLinksLay;
        QLabel* _whatsNewHeader;
        QLabel* _whatsNewText;
        QPushButton* _clearButton;
        QScrollArea* _parent;
        QPixmap _image;

        QUrl _pic1_URL;
        QUrl _text1_URL;
        QUrl _rss_URL;
    };

}

#endif // -------------------------------- end of Linux ------------------------------------// 
