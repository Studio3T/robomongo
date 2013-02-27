#include "robomongo/gui/widgets/workarea/OutputItemHeaderWidget.h"

#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/widgets/workarea/OutputItemContentWidget.h"
#include "robomongo/gui/widgets/workarea/OutputItemWidget.h"
#include "robomongo/gui/widgets/workarea/OutputWidget.h"
#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"

using namespace Robomongo;

OutputItemHeaderWidget::OutputItemHeaderWidget(OutputItemWidget *result, OutputItemContentWidget *output, QWidget *parent) : QFrame(parent),
    itemContent(output),
    item(result),
    _maximized(false),
    _treeMode(true)
{
    setContentsMargins(0,0,0,0);

    // Maximaze button
    _maxButton = new QPushButton;
    _maxButton->setIcon(GuiRegistry::instance().maximizeIcon());
    _maxButton->setFixedSize(18, 18);
    _maxButton->setFlat(true);
    connect(_maxButton, SIGNAL(clicked()), this, SLOT(maximizePart()));

    // Tree mode button
    _treeButton = new QPushButton(this);
    _treeButton->hide();
    _treeButton->setIcon(GuiRegistry::instance().treeIcon());
    _treeButton->setFixedSize(24, 24);
    _treeButton->setFlat(true);
    _treeButton->setCheckable(true);
    _treeButton->setChecked(true);
    connect(_treeButton, SIGNAL(clicked()), this, SLOT(showTree()));

    // Text mode button
    _textButton = new QPushButton;
    _textButton->setIcon(GuiRegistry::instance().textIcon());
    _textButton->setFixedSize(24, 24);
    _textButton->setFlat(true);
    _textButton->setCheckable(true);
    connect(_textButton, SIGNAL(clicked()), this, SLOT(showText()));

    // Custom mode button
    _customButton = new QPushButton(this);
    _customButton->hide();
    _customButton->setIcon(GuiRegistry::instance().customIcon());
    _customButton->setFixedSize(24, 24);
    _customButton->setFlat(true);
    _customButton->setCheckable(true);
    connect(_customButton, SIGNAL(clicked()), this, SLOT(showCustom()));

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
    layout->addWidget(new QLabel(), 1); //placeholder
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
    _customButton->setChecked(false);
    itemContent->showText();
    _treeMode = false;
}

void OutputItemHeaderWidget::showTree()
{
    _textButton->setIcon(GuiRegistry::instance().textIcon());
    _textButton->setChecked(false);
    _treeButton->setIcon(GuiRegistry::instance().treeHighlightedIcon());
    _treeButton->setChecked(true);
    _customButton->setChecked(false);
    itemContent->showTree();
    _treeMode = true;
}

void OutputItemHeaderWidget::showCustom()
{
    _textButton->setIcon(GuiRegistry::instance().textIcon());
    _textButton->setChecked(false);
    _treeButton->setIcon(GuiRegistry::instance().treeIcon());
    _treeButton->setChecked(false);
    _customButton->setChecked(true);
    itemContent->showCustom();
    _treeMode = true;
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
        item->output->restoreSize();
        _maxButton->setIcon(GuiRegistry::instance().maximizeIcon());
    }
    else {
        item->output->maximizePart(item);
        _maxButton->setIcon(GuiRegistry::instance().maximizeHighlightedIcon());
    }

    _maximized = !_maximized;
}

QLabel *OutputItemHeaderWidget::createLabelWithIcon(const QIcon &icon)
{
    QPixmap pixmap = icon.pixmap(16, 16);
    QLabel *label = new QLabel;
    label->setPixmap(pixmap);
    return label;
}

QFrame *OutputItemHeaderWidget::createVerticalLine()
{
    QFrame *vline = new QFrame();
    vline->setFrameShape(QFrame::VLine);
    vline->setFrameShadow(QFrame::Sunken);
    vline->setFixedWidth(5);
    return vline;
}
