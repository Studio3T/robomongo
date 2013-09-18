#pragma once

#include <QWidget>
QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE

#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/gui/widgets/workarea/PagingWidget.h"

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
        void showText();
        void showTree();
        void showTable();
        void showCustom();

    protected:
        virtual void mouseDoubleClickEvent(QMouseEvent *);

    Q_SIGNALS:
        void restoredSize();
        void maximizedPart();

    public Q_SLOTS:        
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
    };
}
