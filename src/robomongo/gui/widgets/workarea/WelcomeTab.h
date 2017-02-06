#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
class QNetworkReply;
class QLabel;
QT_END_NAMESPACE

namespace Robomongo
{

    class WelcomeTab : public QWidget
    {
        Q_OBJECT

    public:
        WelcomeTab(QWidget *parent = nullptr);
        ~WelcomeTab();

    private Q_SLOTS:
        void downloadFinished(QNetworkReply* reply);
        void on_allBlogsButton_clicked();

    private:
        QLabel* _pic1 = nullptr;
        QPushButton* _allBlogsButton = nullptr;

    };

}
