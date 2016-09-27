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
        OutputItemHeaderWidget(OutputItemContentWidget *output, bool multipleResults, bool firstItem, bool lastItem,
                               QWidget *parent = nullptr);
        PagingWidget *paging() const { return _paging; }
        void showText();
        void showTree();
        void showTable();
        void showCustom();
        void applyDockUndockSettings(bool docking);
        void toggleOrientation(Qt::Orientation orientation);

    protected:
        virtual void mouseDoubleClickEvent(QMouseEvent *);

    Q_SIGNALS:
        void restoredSize();
        void maximizedPart();

    public Q_SLOTS:        
        void setTime(const QString &time);
        void setCollection(const QString &collection);
        void maximizeMinimizePart();

    private:
        void updateDockButtonOnToggleOrientation() const;

        QPushButton *_textButton;
        QPushButton *_treeButton;
        QPushButton *_tableButton;
        QPushButton *_customButton;
        QPushButton *_maxButton;
        QFrame *_verticalLine;
        QPushButton *_dockUndockButton;
        Indicator *_collectionIndicator;
        Indicator *_timeIndicator;
        PagingWidget *_paging;

        bool _maximized;
        bool _multipleResults;
        bool _firstItem;
        bool _lastItem;
        Qt::Orientation _orientation;
    };
}
