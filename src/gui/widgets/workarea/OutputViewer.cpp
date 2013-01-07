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

        result->header()->setTime("0.01 ms");

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
    output->setQueryInfo(queryInfo);
    output->outputWidget->update(documents);

    if (GuiRegistry::instance().mainWindow()->textMode())
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
    int s = skip - limit;

    if (s < 0)
        s = 0;

    QueryInfo info(_queryInfo);
    info.limit = limit;
    info.skip = s;

    outputViewer->shell()->query(0, info);
}

void OutputResult::paging_rightClicked(int skip, int limit)
{
    QueryInfo info(_queryInfo);
    info.limit = limit;
    info.skip = skip + limit;

    outputViewer->shell()->query(0, info);
}

OutputResultHeader::OutputResultHeader(OutputResult *result, OutputWidget *output, QWidget *parent) : QFrame(parent),
  outputWidget(output),
  outputResult(result),
  _maximized(false)
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

    QLabel *timeIconLabel = createLabelWithIcon(GuiRegistry::instance().timeIcon());
    QLabel *collectionIconLabel = createLabelWithIcon(GuiRegistry::instance().collectionIcon());

    _timeLabel = new QLabel;
    _collectionLabel = new QLabel;
    _paging = new PagingWidget();

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setContentsMargins(5, 0, 5, 1);
    layout->setSpacing(0);
    layout->addWidget(collectionIconLabel);
    layout->addSpacing(5);
    layout->addWidget(_collectionLabel);
    layout->addSpacing(13);
    layout->addWidget(timeIconLabel);
    layout->addSpacing(5);
    layout->addWidget(_timeLabel);

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
}

void OutputResultHeader::showTree()
{
    _textButton->setIcon(GuiRegistry::instance().textIcon());
    _textButton->setChecked(false);
    _treeButton->setIcon(GuiRegistry::instance().treeHighlightedIcon());
    _treeButton->setChecked(true);
    outputWidget->showTree();
}

void OutputResultHeader::setTime(const QString &time)
{
    _timeLabel->setText(time);
}

void OutputResultHeader::setCollection(const QString collection)
{
    _collectionLabel->setText(collection);
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


