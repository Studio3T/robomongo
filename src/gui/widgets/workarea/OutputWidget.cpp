#include "OutputWidget.h"
#include <QVBoxLayout>
#include "editors/PlainJavaScriptEditor.h"
#include "BsonWidget.h"
#include "Qsci/qscilexerjavascript.h"
#include "editors/JSLexer.h"

using namespace Robomongo;

OutputWidget::OutputWidget(const QString &text) :
    _isTextModeSupported(true),
    _isTreeModeSupported(false),
    _isCustomModeSupported(false),
    _isTextModeInitialized(false),
    _isTreeModeInitialized(false),
    _isCustomModeInitialized(false),
    _text(text),
    _sourceIsText(true),
    _isFirstPartRendered(false)
{
    setup();
}

OutputWidget::OutputWidget(const QList<MongoDocumentPtr> &documents) :
    _isTextModeSupported(true),
    _isTreeModeSupported(true),
    _isCustomModeSupported(false),
    _isTextModeInitialized(false),
    _isTreeModeInitialized(false),
    _isCustomModeInitialized(false),
    _documents(documents),
    _sourceIsText(false),
    _isFirstPartRendered(false)
{
    setup();
}

void OutputWidget::setup()
{
    setContentsMargins(0, 0, 0, 0);
    _stack = new QStackedWidget;

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(_stack);
    setLayout(layout);
}

void OutputWidget::showText()
{
    if (!_isTextModeSupported)
        return;

    if (!_isTextModeInitialized) {
        _log = _configureLogText();

        if (_sourceIsText)
            _log->setText(_text);
        else {
            _log->setText("Loading...");
            JsonPrepareThread *thread = new JsonPrepareThread(_documents);
            connect(thread, SIGNAL(finished()), this, SLOT(jsonPrepared()));
            connect(thread, SIGNAL(partReady(QString)), this, SLOT(jsonPartReady(QString)));
            thread->start();
        }

        _stack->addWidget(_log);
        _isTextModeInitialized = true;
    }

    _stack->setCurrentWidget(_log);
}

void OutputWidget::showTree()
{
    if (!_isTreeModeSupported)
        return;

    if (!_isTreeModeInitialized) {
        _bson = _configureBsonWidget();
        _bson->setDocuments(_documents);
        _stack->addWidget(_bson);
        _isTreeModeInitialized = true;
    }

    _stack->setCurrentWidget(_bson);
}

void OutputWidget::showCustom()
{
    if (!_isCustomModeSupported)
        return;
}

void OutputWidget::jsonPrepared()
{
    // delete thread, that emits this signal
    QThread *thread = static_cast<QThread *>(sender());
    thread->deleteLater();
}

void OutputWidget::jsonPartReady(const QString &json)
{
    _log->setUpdatesEnabled(false);

    if (_isFirstPartRendered)
        _log->append(json);
    else
        _log->setText(json);

    _log->setUpdatesEnabled(true);

    _isFirstPartRendered = true;
}

RoboScintilla *Robomongo::OutputWidget::_configureLogText()
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
    textFont.setPointSize(10);
    textFont.setFamily("Courier");
#endif

    QsciLexerJavaScript * javaScriptLexer = new JSLexer(this);
    javaScriptLexer->setFont(textFont);

    RoboScintilla *_logText = new RoboScintilla;
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

BsonWidget *OutputWidget::_configureBsonWidget()
{
    return new BsonWidget;
}
