#include "OutputViewer.h"
#include <QWebView>
#include <QHBoxLayout>
#include <QtWebKit>
#include <QWebElement>
#include <Qsci/qscilexerjavascript.h>
#include <editors/PlainJavaScriptEditor.h>
#include <editors/JSLexer.h>
#include <QListView>
#include <QTreeView>
#include <domain/MongoShellResult.h>
#include "GuiRegistry.h"

using namespace Robomongo;

OutputViewer::OutputViewer(QWidget *parent) :
    QWidget(parent)
{
    setContentsMargins(0, 0, 0, 0);

    _splitter = new QSplitter(this);
    _splitter->setOrientation(Qt::Vertical);
    _splitter->setHandleWidth(1);
    _splitter->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(_splitter);
    setLayout(layout);
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
            RoboScintilla *logText = _configureLogText();
            logText->setText(result.response);
            _splitter->addWidget(new OutputResult(logText));
        }

        if (result.documents.count() > 0) {
            BsonWidget *widget = _configureBsonWidget();
            widget->setDocuments(result.documents);
            _splitter->addWidget(new OutputResult(widget));
        }
    }
}

void OutputViewer::toggleOrientation()
{
    if (_splitter->orientation() == Qt::Horizontal)
        _splitter->setOrientation(Qt::Vertical);
    else
        _splitter->setOrientation(Qt::Horizontal);

    //_splitter->setHandleWidth(0);
}

RoboScintilla *OutputViewer::_configureLogText()
{
    QFont textFont = font();
#if defined(Q_OS_MAC)
    textFont.setPointSize(12);
    textFont.setFamily("Monaco");
#elif defined(Q_OS_UNIX)
    textFont.setFamily("Monospace");
    textFont.setFixedPitch(true);
    //textFont.setWeight(QFont::Bold);
//    textFont.setPointSize(12);
#elif defined(Q_OS_WIN)
    textFont.setPointSize(12);
    textFont.setFamily("Courier");
#endif

    QsciLexerJavaScript * javaScriptLexer = new JSLexer;
    javaScriptLexer->setFont(textFont);

    RoboScintilla *_logText = new RoboScintilla(_splitter);
    _logText->setLexer(javaScriptLexer);
    _logText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _logText->setAutoIndent(true);
    _logText->setIndentationsUseTabs(false);
    _logText->setIndentationWidth(4);
    _logText->setUtf8(true);
    _logText->installEventFilter(this);
    _logText->setMarginWidth(1, 0); // to hide left gray column
    _logText->setBraceMatching(QsciScintilla::StrictBraceMatch);
    _logText->setFont(textFont);
    _logText->setReadOnly(true);

    _logText->setStyleSheet("QFrame {background-color: rgb(48, 10, 36); border: 1px solid #c7c5c4; border-radius: 4px; margin: 0px; padding: 0px;}");
    //connect(_logText, SIGNAL(linesChanged()), SLOT(ui_logLinesCountChanged()));

    return _logText;
}

BsonWidget *OutputViewer::_configureBsonWidget()
{
    return new BsonWidget(_splitter);
}

OutputResult::OutputResult(QWidget *contentWidget, QWidget *parent)
{
    setContentsMargins(0, 0, 0, 0);
    OutputResultHeader *header = new OutputResultHeader;

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(header);
    layout->addWidget(contentWidget, 1);
    setLayout(layout);
}

OutputResultHeader::OutputResultHeader(QWidget *parent) : QWidget(parent)
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

    QPushButton *text = new QPushButton;
    text->setIcon(GuiRegistry::instance().textIcon());
    text->setFixedSize(18, 18);
    text->setFlat(true);


    QLabel *l = new QLabel("Loaded in 2 ms");
    l->setStyleSheet("font-size:12px;");

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    //layout->addWidget(text);
    layout->addWidget(tree);
    layout->addWidget(text);
    layout->addWidget(max, 0, Qt::AlignRight);
    setLayout(layout);
}


