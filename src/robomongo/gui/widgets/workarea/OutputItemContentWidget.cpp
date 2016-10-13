#include "robomongo/gui/widgets/workarea/OutputItemContentWidget.h"

#include <QVBoxLayout>
#include <Qsci/qscilexerjavascript.h>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/domain/MongoShell.h"

#include "robomongo/gui/widgets/workarea/OutputWidget.h"
#include "robomongo/gui/widgets/workarea/OutputItemHeaderWidget.h"
#include "robomongo/gui/widgets/workarea/JsonPrepareThread.h"
#include "robomongo/gui/widgets/workarea/BsonTreeView.h"
#include "robomongo/gui/widgets/workarea/BsonTreeModel.h"
#include "robomongo/gui/widgets/workarea/BsonTableView.h"
#include "robomongo/gui/widgets/workarea/BsonTableModel.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/gui/widgets/workarea/CollectionStatsTreeWidget.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/editors/JSLexer.h"
#include "robomongo/gui/editors/FindFrame.h"

namespace Robomongo
{
    OutputItemContentWidget::OutputItemContentWidget(ViewMode viewMode, MongoShell *shell, const QString &text, double secs, 
                                                     bool multipleResults, bool firstItem, bool lastItem, QWidget *parent) :
        BaseClass(parent),
        _textView(NULL),
        _bsonTreeview(NULL),
        _thread(NULL),
        _bsonTable(NULL),
        _isTextModeSupported(true),
        _isTreeModeSupported(false),
        _isTableModeSupported(false),
        _isCustomModeSupported(false),
        _isTextModeInitialized(false),
        _isTreeModeInitialized(false),
        _isCustomModeInitialized(false),
        _isTableModeInitialized(false),
        _isFirstPartRendered(false),
        _text(text),
        _shell(shell),
        _outputWidget(dynamic_cast<OutputWidget*>(parentWidget())),
        _initialSkip(0),
        _initialLimit(0),
        _mod(NULL),
        _viewMode(viewMode)
    {
        setup(secs, multipleResults, firstItem, lastItem);
    }

    OutputItemContentWidget::OutputItemContentWidget(ViewMode viewMode, MongoShell *shell, const QString &type,
                                                     const std::vector<MongoDocumentPtr> &documents, const MongoQueryInfo &queryInfo, 
                                                     double secs, bool multipleResults, bool firstItem, bool lastItem, QWidget *parent) :
        BaseClass(parent),
        _textView(NULL),
        _bsonTreeview(NULL),
        _thread(NULL),
        _bsonTable(NULL),
        _isTextModeSupported(true),
        _isTreeModeSupported(true),
        _isTableModeSupported(true),
        _isCustomModeSupported(!type.isEmpty()),
        _isTextModeInitialized(false),
        _isTreeModeInitialized(false),
        _isCustomModeInitialized(false),
        _isTableModeInitialized(false),
        _isFirstPartRendered(false),
        _documents(documents),
        _queryInfo(queryInfo),
        _type(type),
        _shell(shell),
        _initialSkip(queryInfo._skip),
        _initialLimit(queryInfo._limit),
        _outputWidget(dynamic_cast<OutputWidget*>(parentWidget())),
        _mod(NULL),
        _viewMode(viewMode)
    {
        setup(secs, multipleResults, firstItem, lastItem);
    }

    void OutputItemContentWidget::setup(double secs, bool multipleResults, bool firstItem, bool lastItem)
    {      
        setContentsMargins(0, 0, 0, 0);
        _header = new OutputItemHeaderWidget(this, multipleResults, firstItem, lastItem);

        if (_queryInfo._info.isValid()) {
            _header->setCollection(QtUtils::toQString(_queryInfo._info._ns.collectionName()));
            _header->paging()->setBatchSize(_queryInfo._batchSize);
            _header->paging()->setSkip(_queryInfo._skip);
            if (!_queryInfo._limit) {
            _queryInfo._limit = 50;
            }
        }

        _header->setTime(QString("%1 sec.").arg(secs));

        QVBoxLayout *layout = new QVBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(_header);
        _stack = new QStackedWidget;
        layout->addWidget(_stack);
        setLayout(layout);
        configureModel();

        VERIFY(connect(_header->paging(), SIGNAL(refreshed(int, int)), this, SLOT(refresh(int, int))));
        VERIFY(connect(_header->paging(), SIGNAL(leftClicked(int, int)), this, SLOT(paging_leftClicked(int, int))));
        VERIFY(connect(_header->paging(), SIGNAL(rightClicked(int, int)), this, SLOT(paging_rightClicked(int, int))));
        VERIFY(connect(_header, SIGNAL(maximizedPart()), this, SIGNAL(maximizedPart())));
        VERIFY(connect(_header, SIGNAL(restoredSize()), this, SIGNAL(restoredSize())));

        refreshOutputItem();
    }

    void OutputItemContentWidget::paging_leftClicked(int skip, int limit)
    {
        int s = skip - limit;

        if (s < 0)
            s = 0;

        refresh(s, limit);
    }

    void OutputItemContentWidget::refreshOutputItem()
    {
        switch(_viewMode) {
            case Text: showText(); break;
            case Tree: showTree(); break;
            case Table: showTable(); break;
            case Custom: showCustom(); break;
            default: showTree();
        }
    }

    void OutputItemContentWidget::paging_rightClicked(int skip, int limit)
    {
        skip += limit;
        refresh(skip, limit);
    }

    void OutputItemContentWidget::refresh(int skip, int batchSize)
    {
        // Cannot set skip lower than in the text query
        if (skip <  _initialSkip) {
            _header->paging()->setSkip(_initialSkip);
            skip = _initialSkip;
        }

        int skipDelta = skip - _initialSkip;
        int limit = batchSize;

        // If limit is set to 0 it means UNLIMITED number of documents (limited only by batch size)
        // This is according to MongoDB documentation.
        if (_initialLimit != 0) {
            limit = _initialLimit - skipDelta;
            if (limit <= 0)
                limit = -1; // It means that we do not need to load documents

            if (limit > batchSize)
                limit = batchSize;
        }

        MongoQueryInfo info(_queryInfo);
        info._limit = limit;
        info._skip = skip;
        info._batchSize = batchSize;
        _outputWidget->showProgress();
        _shell->query(_outputWidget->resultIndex(this), info);
    }

    void OutputItemContentWidget::update(const MongoQueryInfo &inf, const std::vector<MongoDocumentPtr> &documents)
    {
        _queryInfo = inf;
        _documents = documents;

        _header->paging()->setSkip(_queryInfo._skip);
        _header->paging()->setBatchSize(_queryInfo._batchSize);

        _text.clear();
        _isFirstPartRendered = false;
        markUninitialized();

        if (_bsonTable) {
            _stack->removeWidget(_bsonTable);
            delete _bsonTable;
            _bsonTable = NULL;
        }

        if (_bsonTreeview) {
            _stack->removeWidget(_bsonTreeview);
            delete _bsonTreeview;
            _bsonTreeview = NULL;
        }

        if (_textView) {
            _stack->removeWidget(_textView);
            delete _textView;
            _textView = NULL;
        }
        configureModel();
    }

    void OutputItemContentWidget::showText()
    {
        _viewMode = Text;
        _header->showText();
        if (!_isTextModeSupported)
            return;

        if (!_isTextModeInitialized)
        {
            _textView = configureLogText();
            if (!_text.isEmpty()) {
                _textView->sciScintilla()->setText(_text);
            }
            else {
                if (_documents.size() > 0) {
                    _textView->sciScintilla()->setText("Loading...");
                    _thread = new JsonPrepareThread(_documents, AppRegistry::instance().settingsManager()->uuidEncoding(), AppRegistry::instance().settingsManager()->timeZone());
                    VERIFY(connect(_thread, SIGNAL(partReady(const QString&)), this, SLOT(jsonPartReady(const QString&))));
                    VERIFY(connect(_thread, SIGNAL(finished()), _thread, SLOT(deleteLater())));
                    _thread->start();
                }
            }
            _stack->addWidget(_textView);
            _isTextModeInitialized = true;
        }

        _stack->setCurrentWidget(_textView);
    }

    void OutputItemContentWidget::showTree()
    {
        _viewMode = Tree;
        _header->showTree();
        if (!_isTreeModeSupported) {
            // try to downgrade to text mode
            showText();
            _viewMode = Tree;
            return;
        }

        if (!_isTreeModeInitialized) {
            _bsonTreeview = new BsonTreeView(_shell, _queryInfo);
            _bsonTreeview->setModel(_mod);
            _stack->addWidget(_bsonTreeview);

            if (true == AppRegistry::instance().settingsManager()->autoExpand())
                // Expanding only one level, because on large
                // documents it can take much time
                _bsonTreeview->expand(_mod->index(0, 0, QModelIndex()));

            _isTreeModeInitialized = true;
        }

        _stack->setCurrentWidget(_bsonTreeview);
    }

    void OutputItemContentWidget::showCustom()
    {
        _viewMode = Custom;
        _header->showCustom();

        if (!_isCustomModeSupported) {
            // try to downgrade to tree mode
            showTree();
            _viewMode = Custom;
            return;
        }

        if (!_isCustomModeInitialized) {

            if (_type == "collectionStats") {
                _collectionStats = new CollectionStatsTreeWidget(_documents, NULL);
                _stack->addWidget(_collectionStats);
            }               
            _isCustomModeInitialized = true;
        }

        if (_collectionStats)
            _stack->setCurrentWidget(_collectionStats);
    }

    void OutputItemContentWidget::showTable()
    {
        _viewMode = Table;
        _header->showTable();
        if (!_isTableModeSupported) {
            // try to downgrade to text mode
            showText();
            _viewMode = Table;
            return;
        }

        if (!_isTableModeInitialized) {
            _bsonTable = new BsonTableView(_shell, _queryInfo);
            BsonTableModelProxy *modp = new BsonTableModelProxy(_bsonTable);
            modp->setSourceModel(_mod);
            _bsonTable->setModel(modp);
            _stack->addWidget(_bsonTable);
            _isTableModeInitialized = true;
        }

        _stack->setCurrentWidget(_bsonTable);
    }

    void OutputItemContentWidget::markUninitialized()
    {
        _isTextModeInitialized = false;
        _isTreeModeInitialized = false;
        _isCustomModeInitialized = false;
        _isTableModeInitialized = false;
    }

    void OutputItemContentWidget::applyDockUndockSettings(bool isDocking) const
    {
        _header->applyDockUndockSettings(isDocking);
    }

    void OutputItemContentWidget::toggleOrientation(Qt::Orientation orientation) const
    {
        _header->toggleOrientation(orientation);
    }

    void OutputItemContentWidget::jsonPartReady(const QString &json)
    {
        // check that this is our current thread
        JsonPrepareThread *thread = qobject_cast<JsonPrepareThread *>(sender());
        if (thread && thread != _thread)
        {
            // close previous thread
            thread->stop();
            thread->wait();
        }
        else
        {
            if (_textView)
            {
                if (_isFirstPartRendered)
                    _textView->sciScintilla()->append(json);
                else
                    _textView->sciScintilla()->setText(json);
                _isFirstPartRendered = true;
            }
        }
    }
    
    BsonTreeModel *OutputItemContentWidget::configureModel()
    {
        delete _mod;
        _mod = new BsonTreeModel(_documents, this);
        return _mod;
    }

    FindFrame *Robomongo::OutputItemContentWidget::configureLogText()
    {
        const QFont &textFont = GuiRegistry::instance().font();

        QsciLexerJavaScript *javaScriptLexer = new JSLexer(this);
        javaScriptLexer->setFont(textFont);

        FindFrame *_logText = new FindFrame(this);
        _logText->sciScintilla()->setLexer(javaScriptLexer);
        _logText->sciScintilla()->setTabWidth(4);        
        _logText->sciScintilla()->setAppropriateBraceMatching();
        _logText->sciScintilla()->setFont(textFont);
        _logText->sciScintilla()->setReadOnly(true);
        _logText->sciScintilla()->setWrapMode((QsciScintilla::WrapMode) QsciScintilla::SC_WRAP_NONE);
        _logText->sciScintilla()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        _logText->sciScintilla()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        // Wrap mode turned off because it introduces huge performance problems
        // even for medium size documents.    
        _logText->sciScintilla()->setStyleSheet("QFrame {background-color: rgb(73, 76, 78); border: 1px solid #c7c5c4; border-radius: 0px; margin: 0px; padding: 0px;}");
        return _logText;
    }
}
