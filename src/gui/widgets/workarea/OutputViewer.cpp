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

using namespace Robomongo;

OutputViewer::OutputViewer(bool textMode, QWidget *parent) :
    QWidget(parent),
    _textMode(textMode)
{
    setContentsMargins(0, 0, 0, 0);

    _splitter = new QSplitter;
    _splitter->setOrientation(Qt::Vertical);
    _splitter->setHandleWidth(1);
    _splitter->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(_splitter);
    setLayout(layout);
}

OutputViewer::~OutputViewer()
{
    int t = 56;
}

void OutputViewer::doSomething(const QList<MongoShellResult> &results)
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

        OutputResult *result = new OutputResult(this, output);
        if (GuiRegistry::instance().mainWindow()->textMode())
            result->header()->showText();
        else
            result->header()->showTree();

        result->header()->setTime("0.01 ms");

        if (shellResult.isCollectionValid)
            result->header()->setCollection(shellResult.collectionName);

        _splitter->addWidget(result);
    }
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

OutputResult::OutputResult(OutputViewer *viewer, OutputWidget *output, QWidget *parent) :
    outputWidget(output),
    outputViewer(viewer)
{
    setContentsMargins(0, 0, 0, 0);
    _header = new OutputResultHeader(this, output);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(_header);
    layout->addWidget(output, 1);
    setLayout(layout);
}

OutputResultHeader::OutputResultHeader(OutputResult *result, OutputWidget *output, QWidget *parent) : QWidget(parent),
  outputWidget(output),
  outputResult(result),
  _maximized(false)
{
    setContentsMargins(0,3,0,0);

    _maxButton = new QPushButton;
    _maxButton->setIcon(GuiRegistry::instance().maximizeIcon());
    _maxButton->setFixedSize(18, 18);
    _maxButton->setFlat(true);
    connect(_maxButton, SIGNAL(clicked()), this, SLOT(maximizePart()));

    _treeButton = new QPushButton;
    _treeButton->setIcon(GuiRegistry::instance().treeIcon());
    _treeButton->setFixedSize(18, 18);
    _treeButton->setFlat(true);
    _treeButton->setCheckable(true);
    _treeButton->setChecked(true);
    connect(_treeButton, SIGNAL(clicked()), this, SLOT(showTree()));

    _textButton = new QPushButton;
    _textButton->setIcon(GuiRegistry::instance().textIcon());
    _textButton->setFixedSize(18, 18);
    _textButton->setFlat(true);
    connect(_textButton, SIGNAL(clicked()), this, SLOT(showText()));

    QIcon timeIcon = GuiRegistry::instance().timeIcon();
    QPixmap timePixmap = timeIcon.pixmap(16, 16);
    QLabel *timeIconLabel = new QLabel;
    timeIconLabel->setPixmap(timePixmap);

    QIcon collectionIcon = GuiRegistry::instance().collectionIcon();
    QPixmap collectionPixmap = collectionIcon.pixmap(16, 16);
    QLabel *collectionIconLabel = new QLabel;
    collectionIconLabel->setPixmap(collectionPixmap);

    _timeLabel = new QLabel;
    _collectionLabel = new QLabel;

    QLabel *l = new QLabel("Loaded in 2 ms");
    l->setStyleSheet("font-size:12px;");

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 2);
    layout->setSpacing(0);
    layout->addWidget(collectionIconLabel);
    layout->addSpacing(5);
    layout->addWidget(_collectionLabel);
    layout->addSpacing(13);
    layout->addWidget(timeIconLabel);
    layout->addSpacing(5);
    layout->addWidget(_timeLabel);

    layout->addWidget(new QLabel(), 1); //placeholder

    if (output->isTreeModeSupported())
        layout->addWidget(_treeButton, 0, Qt::AlignRight);

    if (output->isTextModeSupported())
        layout->addWidget(_textButton, 0, Qt::AlignRight);

    layout->addSpacing(5);
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
    _treeButton->setIcon(GuiRegistry::instance().treeIcon());
    outputWidget->showText();
}

void OutputResultHeader::showTree()
{
    _textButton->setIcon(GuiRegistry::instance().textIcon());
    _treeButton->setIcon(GuiRegistry::instance().treeHighlightedIcon());
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


