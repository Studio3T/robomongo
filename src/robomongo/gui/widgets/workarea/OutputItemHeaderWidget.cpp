#include "robomongo/gui/widgets/workarea/OutputItemHeaderWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/widgets/workarea/QueryWidget.h"
#include "robomongo/gui/widgets/workarea/OutputItemContentWidget.h"
#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/settings/SettingsManager.h"

namespace
{
    QFrame *createVerticalLine()
    {
        QFrame *vline = new QFrame();
        vline->setFrameShape(QFrame::VLine);
        vline->setFrameShadow(QFrame::Sunken);
        vline->setFixedWidth(5);
        return vline;
    }
}

namespace Robomongo
{

    OutputItemHeaderWidget::OutputItemHeaderWidget(OutputItemContentWidget *outputItemContentWidget, bool multipleResults, 
                                                   bool firstItem, QWidget *parent) :
        QFrame(parent),
        _maxButton(nullptr), _dockUndockButton(nullptr), _maximized(false), _firstItem(firstItem)
    {
        setContentsMargins(5, 0, 0, 0);

        // Text mode button
        _textButton = new QPushButton(this);
        _textButton->setIcon(GuiRegistry::instance().textIcon());
        _textButton->setToolTip("View results in text mode");
        _textButton->setFixedSize(24, 24);
        _textButton->setFlat(true);
        _textButton->setCheckable(true);

        // Tree mode button
        _treeButton = new QPushButton(this);
        _treeButton->hide();
        _treeButton->setIcon(GuiRegistry::instance().treeIcon());
        _treeButton->setToolTip("View results in tree mode");
        _treeButton->setFixedSize(24, 24);
        _treeButton->setFlat(true);
        _treeButton->setCheckable(true);
        _treeButton->setChecked(true);     

        // Table mode button
        _tableButton = new QPushButton(this);
        _tableButton->hide();
        _tableButton->setIcon(GuiRegistry::instance().tableIcon());
        _tableButton->setToolTip("View results in table mode");
        _tableButton->setFixedSize(24, 24);
        _tableButton->setFlat(true);
        _tableButton->setCheckable(true);
        _tableButton->setChecked(true);       

        // Custom mode button
        _customButton = new QPushButton(this);
        _customButton->hide();
        _customButton->setIcon(GuiRegistry::instance().customIcon());
        _customButton->setToolTip("View results in custom UI");
        _customButton->setFixedSize(24, 24);
        _customButton->setFlat(true);
        _customButton->setCheckable(true);

        // Maximize button - do not crate it if there is only one result
        if (multipleResults) {
            _maxButton = new QPushButton;
            _maxButton->setIcon(GuiRegistry::instance().maximizeIcon());
            _maxButton->setToolTip("Maximize or restore back this output result. You also can double-click on result's header.");
            _maxButton->setFixedSize(18, 18);
            _maxButton->setFlat(true);
            VERIFY(connect(_maxButton, SIGNAL(clicked()), this, SLOT(maximizePart())));
        }

        _dockUndockButton = new QPushButton;
        _dockUndockButton->setIcon(GuiRegistry::instance().undockIcon());
        _dockUndockButton->setToolTip("Undock into separate window");
        _dockUndockButton->setFixedSize(18, 18);
        _dockUndockButton->setFlat(true);
        
        auto dockWidget = dynamic_cast<QueryWidget::CustomDockWidget*>(outputItemContentWidget->parentWidget()->parentWidget());
        auto queryWidget = dockWidget->getQueryWidget();
        VERIFY(connect(_dockUndockButton, SIGNAL(clicked()), queryWidget, SLOT(dockUndock())));
        
        VERIFY(connect(_textButton, SIGNAL(clicked()), outputItemContentWidget, SLOT(showText())));
        VERIFY(connect(_treeButton, SIGNAL(clicked()), outputItemContentWidget, SLOT(showTree())));
        VERIFY(connect(_tableButton, SIGNAL(clicked()), outputItemContentWidget, SLOT(showTable())));
        VERIFY(connect(_customButton, SIGNAL(clicked()), outputItemContentWidget, SLOT(showCustom())));

        _collectionIndicator = new Indicator(GuiRegistry::instance().collectionIcon());
        _timeIndicator = new Indicator(GuiRegistry::instance().timeIcon());
        _paging = new PagingWidget();

        _collectionIndicator->hide();
        _timeIndicator->hide();
        _paging->hide();

        QHBoxLayout *layout = new QHBoxLayout();
        layout->setContentsMargins(2, 0, 5, 1);
        layout->setSpacing(0);

        layout->addWidget(_collectionIndicator);
        layout->addWidget(_timeIndicator);
        QSpacerItem *hSpacer = new QSpacerItem(2000, 24, QSizePolicy::Preferred, QSizePolicy::Minimum);
        layout->addSpacerItem(hSpacer);
        layout->addWidget(_paging);
        layout->addWidget(createVerticalLine());
        layout->addSpacing(2);

        if (outputItemContentWidget->isCustomModeSupported()) {
            layout->addWidget(_customButton, 0, Qt::AlignRight);
            _customButton->show();
        }

        if (outputItemContentWidget->isTreeModeSupported()) {
            layout->addWidget(_treeButton, 0, Qt::AlignRight);
            _treeButton->show();
        }

        if (outputItemContentWidget->isTableModeSupported()) {
            layout->addWidget(_tableButton, 0, Qt::AlignRight);
            _tableButton->show();
        }

        if (outputItemContentWidget->isTextModeSupported())
            layout->addWidget(_textButton, 0, Qt::AlignRight);

        if (multipleResults) {
            layout->addWidget(_maxButton, 0, Qt::AlignRight);
        }

        layout->addSpacing(3);
        _verticalLine = createVerticalLine();
        layout->addWidget(_verticalLine);
        layout->addWidget(_dockUndockButton);
        
        // Add _dockUndockButton only for first header item
        if (!_firstItem) {
            _verticalLine->setHidden(true);
            _dockUndockButton->setHidden(true);
        }

        setLayout(layout);
    }

    void OutputItemHeaderWidget::mouseDoubleClickEvent(QMouseEvent *)
    {
        maximizePart();
    }

    void OutputItemHeaderWidget::showText()
    {
        _textButton->setIcon(GuiRegistry::instance().textHighlightedIcon());
        _textButton->setChecked(true);
        _treeButton->setIcon(GuiRegistry::instance().treeIcon());
        _treeButton->setChecked(false);
        _tableButton->setIcon(GuiRegistry::instance().tableIcon());
        _tableButton->setChecked(false);
        _customButton->setIcon(GuiRegistry::instance().customIcon());
        _customButton->setChecked(false);
    }

    void OutputItemHeaderWidget::showTree()
    {
        _textButton->setIcon(GuiRegistry::instance().textIcon());
        _textButton->setChecked(false);
        _treeButton->setIcon(GuiRegistry::instance().treeHighlightedIcon());
        _treeButton->setChecked(true);
        _tableButton->setIcon(GuiRegistry::instance().tableIcon());
        _tableButton->setChecked(false);
        _customButton->setIcon(GuiRegistry::instance().customIcon());
        _customButton->setChecked(false);
    }

    void OutputItemHeaderWidget::showTable()
    {
        _textButton->setIcon(GuiRegistry::instance().textIcon());
        _textButton->setChecked(false);
        _treeButton->setIcon(GuiRegistry::instance().treeIcon());
        _treeButton->setChecked(false);
        _tableButton->setIcon(GuiRegistry::instance().tableHighlightedIcon());
        _tableButton->setChecked(true);
        _customButton->setIcon(GuiRegistry::instance().customIcon());
        _customButton->setChecked(false);
    }

    void OutputItemHeaderWidget::showCustom()
    {
        _textButton->setIcon(GuiRegistry::instance().textIcon());
        _textButton->setChecked(false);
        _treeButton->setIcon(GuiRegistry::instance().treeIcon());
        _treeButton->setChecked(false);
        _tableButton->setIcon(GuiRegistry::instance().tableIcon());
        _tableButton->setChecked(false);
        _customButton->setIcon(GuiRegistry::instance().customHighlightedIcon());
        _customButton->setChecked(true);
    }

    void OutputItemHeaderWidget::applyDockUndockSettings(bool docking)
    {
        if (docking) {
            _dockUndockButton->setIcon(GuiRegistry::instance().undockIcon());
            _dockUndockButton->setToolTip("Undock into separate window");
        }
        else {
            _dockUndockButton->setIcon(GuiRegistry::instance().dockIcon());
            _dockUndockButton->setToolTip("Dock into main window");
        }
    }

    void OutputItemHeaderWidget::setTime(const QString &time)
    {
        _timeIndicator->setVisible(!time.isEmpty());
        _timeIndicator->setText(time);
    }

    void OutputItemHeaderWidget::setCollection(const QString &collection)
    {
        _collectionIndicator->setVisible(!collection.isEmpty());
        _collectionIndicator->setText(collection);
    }

    void OutputItemHeaderWidget::maximizePart()
    {
        if (_maximized) {
            emit restoredSize();
            _maxButton->setIcon(GuiRegistry::instance().maximizeIcon());
            if (!_firstItem) {
                _verticalLine->setHidden(true);
                _dockUndockButton->setHidden(true);
            }
        }
        else {
            emit maximizedPart();
            _maxButton->setIcon(GuiRegistry::instance().maximizeHighlightedIcon());
            if (!_firstItem) {
                _verticalLine->setVisible(true);
                _dockUndockButton->setVisible(true);
            }
        }

        _maximized = !_maximized;
    }
}
