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

using namespace Robomongo;

OutputViewer::OutputViewer(QWidget *parent) :
    QWidget(parent)
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

    foreach (MongoShellResult result, results) {
        if (!result.response.trimmed().isEmpty()) {
            OutputWidget *output = new OutputWidget(result.response);
            _splitter->addWidget(new OutputResult(output));
            output->showText();
        }

        if (result.documents.count() > 0) {
            OutputWidget *output = new OutputWidget(result.documents);
            _splitter->addWidget(new OutputResult(output));
            output->showTree();
        }
    }
}

void OutputViewer::toggleOrientation()
{
    if (_splitter->orientation() == Qt::Horizontal)
        _splitter->setOrientation(Qt::Vertical);
    else
        _splitter->setOrientation(Qt::Horizontal);
}

OutputResult::OutputResult(OutputWidget *output, QWidget *parent) :
    outputWidget(output)
{
    setContentsMargins(0, 0, 0, 0);
    OutputResultHeader *header = new OutputResultHeader(output);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(header);
    layout->addWidget(output, 1);
    setLayout(layout);
}

OutputResultHeader::OutputResultHeader(OutputWidget *output, QWidget *parent) : QWidget(parent),
  outputWidget(output)
{
    setContentsMargins(0,3,0,0);

    QPushButton *max = new QPushButton;
    max->setIcon(GuiRegistry::instance().maximizeIcon());
    max->setFixedSize(18, 18);
    max->setFlat(true);

    QPushButton *tree = new QPushButton;
    tree->setIcon(GuiRegistry::instance().treeIcon());
    tree->setFixedSize(18, 18);
    tree->setFlat(true);
    tree->setCheckable(true);
    tree->setChecked(true);
    connect(tree, SIGNAL(clicked()), this, SLOT(showTree()));

    QPushButton *text = new QPushButton;
    text->setIcon(GuiRegistry::instance().textIcon());
    text->setFixedSize(18, 18);
    text->setFlat(true);
    connect(text, SIGNAL(clicked()), this, SLOT(showText()));

    QLabel *l = new QLabel("Loaded in 2 ms");
    l->setStyleSheet("font-size:12px;");

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    if (output->isTreeModeSupported())
        layout->addWidget(tree);

    if (output->isTextModeSupported())
        layout->addWidget(text);

    layout->addWidget(max, 0, Qt::AlignRight);
    setLayout(layout);
}

void OutputResultHeader::showText()
{
    outputWidget->showText();
}

void OutputResultHeader::showTree()
{
    outputWidget->showTree();
}


