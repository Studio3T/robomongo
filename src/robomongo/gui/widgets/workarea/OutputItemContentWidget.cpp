#include "robomongo/gui/widgets/workarea/OutputItemContentWidget.h"

#include <QVBoxLayout>
#include <Qsci/qscilexerjavascript.h>

#include "robomongo/gui/widgets/workarea/BsonWidget.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/gui/widgets/workarea/CollectionStatsTreeWidget.h"
#include "robomongo/gui/editors/JSLexer.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/settings/SettingsManager.h"

using namespace Robomongo;

OutputItemContentWidget::OutputItemContentWidget(MongoShell *shell, const QString &text) :
    _isTextModeSupported(true),
    _isTreeModeSupported(false),
    _isCustomModeSupported(false),
    _text(text),
    _sourceIsText(true),
    _shell(shell)
{
    setup();
}

OutputItemContentWidget::OutputItemContentWidget(MongoShell *shell, const QList<MongoDocumentPtr> &documents, const MongoQueryInfo &queryInfo) :
    _isTextModeSupported(true),
    _isTreeModeSupported(true),
    _isCustomModeSupported(false),
    _documents(documents),
    _queryInfo(queryInfo),
    _sourceIsText(false),
    _shell(shell)
{
    setup();
}

OutputItemContentWidget::OutputItemContentWidget(MongoShell *shell, const QString &type, const QList<MongoDocumentPtr> &documents, const MongoQueryInfo &queryInfo) :
    _isTextModeSupported(true),
    _isTreeModeSupported(true),
    _isCustomModeSupported(true),
    _documents(documents),
    _queryInfo(queryInfo),
    _sourceIsText(false),
    _type(type),
    _shell(shell)
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
    _sourceIsText = true;
    _isFirstPartRendered = false;
    markUninitialized();

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
    _documents.clear();
    _documents = documents;
    _sourceIsText = false;
    _isFirstPartRendered = false;
    markUninitialized();

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
    markUninitialized();

    _isFirstPartRendered = false;
    _log = NULL;
    _bson = NULL;
    _thread = NULL;

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

                UUIDEncoding uuidEncoding = AppRegistry::instance().settingsManager()->uuidEncoding();
                _thread = new JsonPrepareThread(_documents, uuidEncoding);
                connect(_thread, SIGNAL(done()), this, SLOT(jsonPrepared()));
                connect(_thread, SIGNAL(partReady(QString)), this, SLOT(jsonPartReady(QString)));
                connect(_thread, SIGNAL(finished()), _thread, SLOT(deleteLater()));
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
        _bson->setDocuments(_documents, _queryInfo);
        _stack->addWidget(_bson);
        _isTreeModeInitialized = true;
    }

    _stack->setCurrentWidget(_bson);
}

void OutputItemContentWidget::showCustom()
{
    if (!_isCustomModeSupported) {
        // try to downgrade to tree mode
        showTree();
        return;
    }

    QWidget *customWidget = NULL;

    if (!_isCustomModeInitialized) {

        if (_type == "collectionStats") {
            _collectionStats = new CollectionStatsTreeWidget(_shell);
            _collectionStats->setDocuments(_documents);
            customWidget = _collectionStats;
        }

        if (customWidget)
            _stack->addWidget(_collectionStats);
        _isCustomModeInitialized = true;
    }

    if (_collectionStats)
        _stack->setCurrentWidget(_collectionStats);
}

void OutputItemContentWidget::markUninitialized()
{
    _isTextModeInitialized = false;
    _isTreeModeInitialized = false;
    _isCustomModeInitialized = false;
}

void OutputItemContentWidget::jsonPrepared()
{
    // seems that it is wrong to call any method on thread,
    // because thread already can be disposed.
    // QThread *thread = static_cast<QThread *>(sender());
    // thread->quit();
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

    QsciLexerJavaScript *javaScriptLexer = new JSLexer(this);
    javaScriptLexer->setFont(textFont);

    RoboScintilla *_logText = new RoboScintilla;
    _logText->setLexer(javaScriptLexer);
    _logText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _logText->setAutoIndent(true);
    _logText->setIndentationsUseTabs(false);
    _logText->setIndentationWidth(4);
    _logText->setTabWidth(4);
    _logText->setUtf8(true);
    _logText->installEventFilter(this);
    _logText->setCaretForegroundColor(QColor("#FFFFFF"));
    _logText->setMarginWidth(1, 0); // to hide left gray column
    _logText->setMatchedBraceBackgroundColor(QColor(73, 76, 78));
    _logText->setMatchedBraceForegroundColor(QColor("#FF8861")); //1AB0A6
    _logText->setBraceMatching(QsciScintilla::StrictBraceMatch);
    _logText->setFont(textFont);
    _logText->setReadOnly(true);

    // Wrap mode turned off because it introduces huge performance problems
    // even for medium size documents.
    _logText->setWrapMode((QsciScintilla::WrapMode)QsciScintilla::SC_WRAP_NONE);

    _logText->setStyleSheet("QFrame {background-color: rgb(73, 76, 78); border: 1px solid #c7c5c4; border-radius: 0px; margin: 0px; padding: 0px;}");
    return _logText;
}

BsonWidget *OutputItemContentWidget::_configureBsonWidget()
{
    return new BsonWidget(_shell);
}
