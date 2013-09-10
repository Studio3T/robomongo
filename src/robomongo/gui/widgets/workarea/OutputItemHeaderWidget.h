#pragma once

#include <QWidget>
QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE

#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/gui/widgets/workarea/PagingWidget.h"
#include "robomongo/gui/ViewMode.h"

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
        OutputItemHeaderWidget(OutputItemWidget *result, OutputItemContentWidget *output, QWidget *parent = 0);
        OutputItemContentWidget *itemContent;
        OutputItemWidget *item;
        PagingWidget *paging() const { return _paging; }

    protected:
        virtual void mouseDoubleClickEvent(QMouseEvent *);

    public Q_SLOTS:
        void showText();
        void showTree();
        void showTable();
        void showCustom();
        void refreshOutputItem();
        void showIn(ViewMode mode);
        void setTime(const QString &time);
        void setCollection(const QString &collection);
        void maximizePart();

    private:
        QPushButton *_textButton;
        QPushButton *_treeButton;
        QPushButton *_tableButton;
        QPushButton *_customButton;
        QPushButton *_maxButton;
        Indicator *_collectionIndicator;
        Indicator *_timeIndicator;
        PagingWidget *_paging;
        bool _maximized;
        ViewMode _viewMode;
    };
}
