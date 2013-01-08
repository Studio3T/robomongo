#include "OutputItemContentWidget.h"
#include <QVBoxLayout>
#include "editors/PlainJavaScriptEditor.h"
#include "BsonWidget.h"
#include "Qsci/qscilexerjavascript.h"
#include "editors/JSLexer.h"

using namespace Robomongo;

OutputItemContentWidget::OutputItemContentWidget(const QString &text) :
    _isTextModeSupported(true),
    _isTreeModeSupported(false),
    _isCustomModeSupported(false),
    _isTextModeInitialized(false),
    _isTreeModeInitialized(false),
    _isCustomModeInitialized(false),
    _text(text),
    _sourceIsText(true),
    _isFirstPartRendered(false),
    _log(NULL),
    _bson(NULL),
    _thread(NULL)

{
    setup();
}

OutputItemContentWidget::OutputItemContentWidget(const QList<MongoDocumentPtr> &documents) :
    _isTextModeSupported(true),
    _isTreeModeSupported(true),
    _isCustomModeSupported(false),
    _isTextModeInitialized(false),
    _isTreeModeInitialized(false),
    _isCustomModeInitialized(false),
    _documents(documents),
    _sourceIsText(false),
    _isFirstPartRendered(false),
    _log(NULL),
    _bson(NULL),
    _thread(NULL)
{
    setup();
}

OutputItemContentWidget::~OutputItemContentWidget()
{
/*    if (_thread)
        _thread->exit = true;*/
}

void OutputItemContentWidget::update(const QString &text)
{
    _text = text;
    _isTextModeInitialized = false;
    _isTreeModeInitialized = false;
    _isCustomModeInitialized = false;
    _sourceIsText = true;
    _isFirstPartRendered = false;

    if (_bson) {
        _stack->removeWidget(_bson);
        delete _bson;
        _bson = NULL;
    }

    if (_log) {
        _stack->removeWidget(_log);
        delete _log;
        _log = NULL;
    }
}

void OutputItemContentWidget::update(const QList<MongoDocumentPtr> &documents)
{
    _isTextModeInitialized = false;
    _isTreeModeInitialized = false;
    _isCustomModeInitialized = false;
    _documents.clear();
    _documents = documents;
    _sourceIsText = false;
    _isFirstPartRendered = false;

    if (_bson) {
        _stack->removeWidget(_bson);
        delete _bson;
        _bson = NULL;
    }

    if (_log) {
        _stack->removeWidget(_log);
        delete _log;
        _log = NULL;
    }
}

void OutputItemContentWidget::setup()
{
    setContentsMargins(0, 0, 0, 0);
    _stack = new QStackedWidget;

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(_stack);
    setLayout(layout);
}

void OutputItemContentWidget::showText()
{
    if (!_isTextModeSupported)
        return;

    if (!_isTextModeInitialized) {
        _log = _configureLogText();

        if (_sourceIsText)
            _log->setText(_text);
        else {

            if (_documents.count() > 0) {
                _log->setText("Loading...");

                _thread = new JsonPrepareThread(_documents);
                connect(_thread, SIGNAL(done()), this, SLOT(jsonPrepared()));
                connect(_thread, SIGNAL(partReady(QString)), this, SLOT(jsonPartReady(QString)));
                _thread->start();
            }
        }

        _stack->addWidget(_log);
        _isTextModeInitialized = true;
    }

    _stack->setCurrentWidget(_log);
}

void OutputItemContentWidget::showTree()
{
    if (!_isTreeModeSupported) {
        // try to downgrade to text mode
        showText();
        return;
    }

    if (!_isTreeModeInitialized) {
        _bson = _configureBsonWidget();
        _bson->setDocuments(_documents);
        _stack->addWidget(_bson);
        _isTreeModeInitialized = true;
    }

    _stack->setCurrentWidget(_bson);
}

void OutputItemContentWidget::showCustom()
{
    if (!_isCustomModeSupported)
        return;
}

void OutputItemContentWidget::jsonPrepared()
{
    // delete thread, that emits this signal
    QThread *thread = static_cast<QThread *>(sender());
    thread->deleteLater();
}

void OutputItemContentWidget::jsonPartReady(const QString &json)
{
    // check that this is our current thread
    JsonPrepareThread *thread = (JsonPrepareThread *) sender();
    if (thread != _thread) {
        // close previous thread
        thread->exit = true;
        thread->wait();
        return;
    }

    if (_log) {
        _log->setUpdatesEnabled(false);

        if (_isFirstPartRendered)
            _log->append(json);
        else
            _log->setText(json);

        _log->setUpdatesEnabled(true);

        _isFirstPartRendered = true;
    }
}

RoboScintilla *Robomongo::OutputItemContentWidget::_configureLogText()
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

    _logText->setStyleSheet("QFrame {background-color: rgb(48, 10, 36); border: 1px solid #c7c5c4; border-radius: 0px; margin: 0px; padding: 0px;}");
    //connect(_logText, SIGNAL(linesChanged()), SLOT(ui_logLinesCountChanged()));

    return _logText;
}

BsonWidget *OutputItemContentWidget::_configureBsonWidget()
{
    return new BsonWidget;
}
