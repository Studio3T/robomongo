#pragma once

#include <QWidget>
QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

namespace Robomongo
{
    class PagingWidget : public QWidget
    {
        Q_OBJECT

    public:
        typedef QWidget BaseClass;
        enum {pageLimit = 50};
        PagingWidget(QWidget *parent = NULL);
        void setSkip(int skip);
        void setBatchSize(int limit);

    Q_SIGNALS:
        void leftClicked(int skip, int limit);
        void rightClicked(int skip, int limit);
        void refreshed(int skip, int limit);

    private Q_SLOTS:
        void leftButton_clicked();
        void rightButton_clicked();
        void refresh();

    private:
        QLineEdit *_skipEdit;
        QLineEdit *_batchSizeEdit;
    };
}
