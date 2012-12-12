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

using namespace Robomongo;

OutputViewer::OutputViewer(QWidget *parent) :
    QWidget(parent)
{
    setContentsMargins(0, 0, 0, 0);

    _splitter = new QSplitter(this);
    _splitter->setOrientation(Qt::Vertical);
    _splitter->setHandleWidth(4);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setMargin(0);
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
            _splitter->addWidget(logText);
        }

        if (result.documents.count() > 0) {
            BsonWidget *widget = _configureBsonWidget();
            widget->setDocuments(result.documents);
            _splitter->addWidget(widget);
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
