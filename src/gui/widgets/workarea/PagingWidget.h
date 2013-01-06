#ifndef PAGINGWIDGET_H
#define PAGINGWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QIcon>
#include <QLineEdit>
#include <QPushButton>

namespace Robomongo
{
    class PagingWidget : public QWidget
    {
        Q_OBJECT
    public:
        PagingWidget();
        void setSkip(int skip);
        void setLimit(int limit);

    signals:
        void leftClicked(int skip, int limit);
        void rightClicked(int skip, int limit);

    private:

        QPushButton *createButtonWithIcon(const QIcon &icon);
        QLineEdit *_skipEdit;
        QLineEdit *_limitEdit;
    };
}


#endif // PAGINGWIDGET_H
