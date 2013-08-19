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
        enum {pageLimit=50};
        PagingWidget();
        void setSkip(int skip);
        void setLimit(int limit);

    Q_SIGNALS:
        void leftClicked(int skip, int limit);
        void rightClicked(int skip, int limit);

    private Q_SLOTS:
        void leftButton_clicked();
        void rightButton_clicked();

    private:
        QLineEdit *_skipEdit;
        QLineEdit *_limitEdit;
    };
}
