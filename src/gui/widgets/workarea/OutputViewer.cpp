#include "OutputViewer.h"
#include <QHBoxLayout>
#include <Qsci/qscilexerjavascript.h>
#include <editors/PlainJavaScriptEditor.h>
#include <editors/JSLexer.h>
#include <QListView>
#include <QTreeView>
#include <domain/MongoShellResult.h>
#include "GuiRegistry.h"
#include "OutputWidget.h"
#include "MainWindow.h"
#include "PagingWidget.h"
#include "domain/MongoShell.h"

using namespace Robomongo;

OutputViewer::OutputViewer(bool textMode, MongoShell *shell, QWidget *parent) :
    QFrame(parent),
    _textMode(textMode),
    _shell(shell),
    _splitter(NULL)
{
    QString style = QString("Robomongo--OutputViewer { background-color: %1; border-radius: 6px; }")
        .arg(QColor("#083047").lighter(660).name());
//            .arg("white");

    //setStyleSheet(style);

/*    setAutoFillBackground(true);
    QPalette p(palette());
    // Set background colour to black
    p.setColor(QPalette::Background, QColor("#083047").lighter(660));
    setPalette(p);
    */

    _splitter = new QSplitter;
    _splitter->setOrientation(Qt::Vertical);
    _splitter->setHandleWidth(1);
    _splitter->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(4, 0, 4, 4);
    layout->setSpacing(0);
    layout->addWidget(_splitter);
    setLayout(layout);
}

OutputViewer::~OutputViewer()
{

}

void OutputViewer::present(const QList<MongoShellResult> &results)
{
    int count = _splitter->count();

    for (int i = 0; i < count; i++) {
        QWidget *widget = _splitter->widget(i);
        widget->hide();
        widget->deleteLater();
    }

    foreach (MongoShellResult shellResult, results) {
        OutputWidget *output = NULL;

        if (!shellResult.response.trimmed().isEmpty()) {
            output = new OutputWidget(shellResult.response);
        }

        if (shellResult.documents.count() > 0) {
            output = new OutputWidget(shellResult.documents);
        }

        OutputResult *result = new OutputResult(this, output, shellResult.queryInfo);
        if (GuiRegistry::instance().mainWindow()->textMode())
            result->header()->showText();
        else
            result->header()->showTree();

        double secs = shellResult.elapsedms / (double) 1000;

        result->header()->setTime(QString("%1 sec.").arg(secs));

        if (!shellResult.queryInfo.isNull) {
            result->header()->setCollection(shellResult.queryInfo.collectionName);
            result->header()->paging()->setLimit(shellResult.queryInfo.limit);
            result->header()->paging()->setSkip(shellResult.queryInfo.skip);
        }

        _splitter->addWidget(result);
    }
}

void OutputViewer::updatePart(int partIndex, const QueryInfo &queryInfo, const QList<MongoDocumentPtr> &documents)
{
    if (partIndex >= _splitter->count())
        return;

    OutputResult *output = (OutputResult *) _splitter->widget(partIndex);

    output->header()->paging()->setSkip(queryInfo.skip);
    output->header()->paging()->setLimit(queryInfo.limit);
    output->setQueryInfo(queryInfo);
    output->outputWidget->update(documents);

    if (!output->header()->treeMode())
        output->header()->showText();
    else
        output->header()->showTree();
}

void OutputViewer::toggleOrientation()
{
    if (_splitter->orientation() == Qt::Horizontal)
        _splitter->setOrientation(Qt::Vertical);
    else
        _splitter->setOrientation(Qt::Horizontal);
}

void OutputViewer::enterTreeMode()
{
    int count = _splitter->count();
    for (int i = 0; i < count; i++) {
        OutputResult *widget = (OutputResult *) _splitter->widget(i);
        widget->header()->showTree();
    }
}

void OutputViewer::enterTextMode()
{
    int count = _splitter->count();
    for (int i = 0; i < count; i++) {
        OutputResult *widget = (OutputResult *) _splitter->widget(i);
        widget->header()->showText();
    }
}

void OutputViewer::maximizePart(OutputResult *result)
{
    int count = _splitter->count();
    for (int i = 0; i < count; i++) {
        OutputResult *widget = (OutputResult *) _splitter->widget(i);

        if (widget != result)
            widget->hide();
    }
}

void OutputViewer::restoreSize()
{
    int count = _splitter->count();
    for (int i = 0; i < count; i++) {
        OutputResult *widget = (OutputResult *) _splitter->widget(i);
        widget->show();
    }
}

int OutputViewer::resultIndex(OutputResult *result)
{
    return _splitter->indexOf(result);
}

OutputResult::OutputResult(OutputViewer *viewer, OutputWidget *output, const QueryInfo &info, QWidget *parent) :
    outputWidget(output),
    outputViewer(viewer),
    _queryInfo(info)
{
    setContentsMargins(0, 0, 0, 0);
    _header = new OutputResultHeader(this, output);

    QFrame *hline = new QFrame();
    hline->setFrameShape(QFrame::HLine);
    hline->setFrameShadow(QFrame::Sunken);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 1, 0, 0);
    layout->setSpacing(0);
//    layout->addWidget(hline);
    layout->addWidget(_header);
    layout->addWidget(output, 1);
    setLayout(layout);

    connect(_header->paging(), SIGNAL(leftClicked(int,int)), this, SLOT(paging_leftClicked(int,int)));
    connect(_header->paging(), SIGNAL(rightClicked(int,int)), this, SLOT(paging_rightClicked(int,int)));

}

void OutputResult::setQueryInfo(const QueryInfo &queryInfo)
{
    _queryInfo = queryInfo;
}

void OutputResult::paging_leftClicked(int skip, int limit)
{
    if (limit > 50)
        limit = 50;

    int s = skip - limit;

    if (s < 0)
        s = 0;

    QueryInfo info(_queryInfo);
    info.limit = limit;
    info.skip = s;

    outputViewer->shell()->query(outputViewer->resultIndex(this), info);
}

void OutputResult::paging_rightClicked(int skip, int limit)
{
    if (limit > 50)
        limit = 50;

    QueryInfo info(_queryInfo);
    info.limit = limit;
    info.skip = skip + limit;

    outputViewer->shell()->query(outputViewer->resultIndex(this), info);
}

OutputResultHeader::OutputResultHeader(OutputResult *result, OutputWidget *output, QWidget *parent) : QFrame(parent),
    outputWidget(output),
    outputResult(result),
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
    _treeButton = new QPushButton;
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

    if (output->isTreeModeSupported())
        layout->addWidget(_treeButton, 0, Qt::AlignRight);

    if (output->isTextModeSupported())
        layout->addWidget(_textButton, 0, Qt::AlignRight);

    layout->addSpacing(3);
    layout->addWidget(createVerticalLine());
    layout->addWidget(_maxButton, 0, Qt::AlignRight);
    setLayout(layout);
}

void OutputResultHeader::mouseDoubleClickEvent(QMouseEvent *)
{
    maximizePart();
}

void OutputResultHeader::showText()
{
    _textButton->setIcon(GuiRegistry::instance().textHighlightedIcon());
    _textButton->setChecked(true);
    _treeButton->setIcon(GuiRegistry::instance().treeIcon());
    _treeButton->setChecked(false);
    outputWidget->showText();
    _treeMode = false;
}

void OutputResultHeader::showTree()
{
    _textButton->setIcon(GuiRegistry::instance().textIcon());
    _textButton->setChecked(false);
    _treeButton->setIcon(GuiRegistry::instance().treeHighlightedIcon());
    _treeButton->setChecked(true);
    outputWidget->showTree();
    _treeMode = true;
}

void OutputResultHeader::setTime(const QString &time)
{
    _timeIndicator->setVisible(!time.isEmpty());
    _timeIndicator->setText(time);
}

void OutputResultHeader::setCollection(const QString &collection)
{
    _collectionIndicator->setVisible(!collection.isEmpty());
    _collectionIndicator->setText(collection);
}

void OutputResultHeader::maximizePart()
{
    if (_maximized) {
        outputResult->outputViewer->restoreSize();
        _maxButton->setIcon(GuiRegistry::instance().maximizeIcon());
    }
    else {
        outputResult->outputViewer->maximizePart(outputResult);
        _maxButton->setIcon(GuiRegistry::instance().maximizeHighlightedIcon());
    }

    _maximized = !_maximized;
}

QLabel *OutputResultHeader::createLabelWithIcon(const QIcon &icon)
{
    QPixmap pixmap = icon.pixmap(16, 16);
    QLabel *label = new QLabel;
    label->setPixmap(pixmap);
    return label;
}

QFrame *OutputResultHeader::createVerticalLine()
{
    QFrame *vline = new QFrame();
    vline->setFrameShape(QFrame::VLine);
    vline->setFrameShadow(QFrame::Sunken);
    vline->setFixedWidth(5);
    return vline;
}

Indicator::Indicator(const QIcon &icon)
{
    QLabel *iconLabel = createLabelWithIcon(icon);
    _label = new QLabel();

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(iconLabel);
    layout->addSpacing(7);
    layout->addWidget(_label);
    layout->addSpacing(14);
    setLayout(layout);
}

void Indicator::setText(const QString &text)
{
    _label->setText(text);
}

QLabel *Indicator::createLabelWithIcon(const QIcon &icon)
{
    QPixmap pixmap = icon.pixmap(16, 16);
    QLabel *label = new QLabel;
    label->setPixmap(pixmap);
    return label;
}
