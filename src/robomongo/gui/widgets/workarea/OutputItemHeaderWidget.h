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
    class Indicator;

    class OutputItemHeaderWidget : public QFrame
    {
        Q_OBJECT

    public:
        OutputItemHeaderWidget(OutputItemContentWidget *output, QWidget *parent = 0);
        PagingWidget *paging() const { return _paging; }
        ViewMode viewMode() const { return _viewMode; }
    protected:
        virtual void mouseDoubleClickEvent(QMouseEvent *);

    Q_SIGNALS:
        void restoredSize();
        void maximizedPart(OutputItemContentWidget *);

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
        OutputItemContentWidget *_itemContent;
        bool _maximized;
        ViewMode _viewMode;
    };
}
