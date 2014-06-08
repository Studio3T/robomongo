#include "robomongo/gui/widgets/workarea/OutputItemHeaderWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/GuiRegistry.h"
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

    OutputItemHeaderWidget::OutputItemHeaderWidget(OutputItemContentWidget *output, QWidget *parent) : 
        QFrame(parent),
        _maximized(false)
    {
        setContentsMargins(5,0,0,0);
        
        // Maximize button
        _maxButton = new QPushButton;
        _maxButton->setIcon(GuiRegistry::instance().maximizeIcon());
        _maxButton->setToolTip("Maximize or restore back this output result. You also can double-click on result's header.");
        _maxButton->setFixedSize(18, 18);
        _maxButton->setFlat(true);
        VERIFY(connect(_maxButton, SIGNAL(clicked()), this, SLOT(maximizePart())));

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

        VERIFY(connect(_textButton, SIGNAL(clicked()), output, SLOT(showText())));
        VERIFY(connect(_treeButton, SIGNAL(clicked()), output, SLOT(showTree())));
        VERIFY(connect(_tableButton, SIGNAL(clicked()), output, SLOT(showTable())));
        VERIFY(connect(_customButton, SIGNAL(clicked()), output, SLOT(showCustom())));

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

        if (output->isCustomModeSupported()) {
            layout->addWidget(_customButton, 0, Qt::AlignRight);
            _customButton->show();
        }

        if (output->isTreeModeSupported()) {
            layout->addWidget(_treeButton, 0, Qt::AlignRight);
            _treeButton->show();
        }

        if (output->isTableModeSupported()) {
            layout->addWidget(_tableButton, 0, Qt::AlignRight);
            _tableButton->show();
        }

        if (output->isTextModeSupported())
            layout->addWidget(_textButton, 0, Qt::AlignRight);

        layout->addSpacing(3);
        layout->addWidget(createVerticalLine());
        layout->addWidget(_maxButton, 0, Qt::AlignRight);
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
            //item->_output->restoreSize();
            emit restoredSize();
            _maxButton->setIcon(GuiRegistry::instance().maximizeIcon());
        }
        else {
            //item->_output->maximizePart(item);
            emit maximizedPart();
            _maxButton->setIcon(GuiRegistry::instance().maximizeHighlightedIcon());
        }

        _maximized = !_maximized;
    }

}
