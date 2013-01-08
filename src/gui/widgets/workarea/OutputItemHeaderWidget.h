#ifndef OUTPUTITEMHEADERWIDGET_H
#define OUTPUTITEMHEADERWIDGET_H

#include <QWidget>
#include <QSplitter>
#include <editors/PlainJavaScriptEditor.h>
#include "BsonWidget.h"
#include <domain/MongoShellResult.h>
#include "PagingWidget.h"

namespace Robomongo
{
    class OutputItemContentWidget;
    class OutputItemWidget;
    class OutputWidget;
    class Indicator;

    class OutputItemHeaderWidget : public QFrame
    {
        Q_OBJECT
    public:
        explicit OutputItemHeaderWidget(OutputItemWidget *result, OutputItemContentWidget *output, QWidget *parent = 0);
        OutputItemContentWidget *itemContent;
        OutputItemWidget *item;
        PagingWidget *paging() const { return _paging; }
        bool treeMode() const { return _treeMode; }

    protected:
        void mouseDoubleClickEvent(QMouseEvent *);

    public slots:
        void showText();
        void showTree();
        void setTime(const QString &time);
        void setCollection(const QString &collection);
        void maximizePart();

    private:
        QLabel *createLabelWithIcon(const QIcon &icon);
        QFrame *createVerticalLine();
        QPushButton *_treeButton;
        QPushButton *_textButton;
        QPushButton *_maxButton;
        Indicator *_collectionIndicator;
        Indicator *_timeIndicator;
        PagingWidget *_paging;
        bool _maximized;
        bool _treeMode;
    };
}

#endif // OUTPUTITEMHEADERWIDGET_H
