#include "robomongo/gui/widgets/workarea/OutputItemContentWidget.h"

#include <QVBoxLayout>
#include <Qsci/qscilexerjavascript.h>
#include <QAction>
#include <QMenu>
#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QMessageBox>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/domain/MongoShell.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/mongodb/MongoWorker.h"

#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"
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
#include "robomongo/gui/dialogs/DocumentTextEditor.h"
#include "robomongo/core/utils/BsonUtils.h" 
#include "robomongo/gui/utils/DialogUtils.h"

namespace Robomongo
{
    OutputItemContentWidget::OutputItemContentWidget(OutputWidget *out, ViewMode viewMode, MongoShell *shell, const QString &text, double secs, QWidget *parent) :
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
        _initialSkip(0),
        _initialLimit(0),
        _out(out),
        _mod(NULL),
        _viewMode(viewMode)
    {
        setup(secs);
    }

    OutputItemContentWidget::OutputItemContentWidget(OutputWidget *out, ViewMode viewMode, MongoShell *shell, const QString &type, const std::vector<MongoDocumentPtr> &documents, const MongoQueryInfo &queryInfo, double secs, QWidget *parent) :
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
        _out(out),
        _mod(NULL),
        _viewMode(viewMode)
    {
        setup(secs);
    }

    void OutputItemContentWidget::setup(double secs)
    {      
        setContentsMargins(0, 0, 0, 0);
        _header = new OutputItemHeaderWidget(this);       

        if (_queryInfo._collectionInfo.isValid()) {
            _header->setCollection(QtUtils::toQString(_queryInfo._collectionInfo._ns.collectionName()));
            _header->paging()->setBatchSize(_queryInfo._batchSize);
            _header->paging()->setSkip(_queryInfo._skip);
            if(!_queryInfo._limit){
            _queryInfo._limit=50;
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

        VERIFY(connect(_header->paging(), SIGNAL(refreshed(int,int)), this, SLOT(refresh(int,int))));        
        VERIFY(connect(_header->paging(), SIGNAL(leftClicked(int,int)), this, SLOT(paging_leftClicked(int,int))));
        VERIFY(connect(_header->paging(), SIGNAL(rightClicked(int,int)), this, SLOT(paging_rightClicked(int,int))));
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
        _out->showProgress();
        _shell->server()->query(_out->resultIndex(this), info);
    }

    void OutputItemContentWidget::update(const MongoQueryInfo &inf,const std::vector<MongoDocumentPtr> &documents)
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
            return;
        }

        if (!_isTreeModeInitialized) {
            _bsonTreeview = new BsonTreeView(this);
            _bsonTreeview->setModel(_mod);
            _stack->addWidget(_bsonTreeview);
            if (AppRegistry::instance().settingsManager()->autoExpand())
                _bsonTreeview->expandNode(_mod->index(0, 0, QModelIndex()));
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
            return;
        }

        if (!_isTableModeInitialized) {
            _bsonTable = new BsonTableView(this);            
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

    FindFrame *OutputItemContentWidget::configureLogText()
    {
        const QFont &textFont = GuiRegistry::instance().font();

        QsciLexerJavaScript *javaScriptLexer = new JSLexer(this);
        javaScriptLexer->setFont(textFont);

        FindFrame *_logText = new FindFrame(this);
        _logText->sciScintilla()->setLexer(javaScriptLexer);
        _logText->sciScintilla()->setTabWidth(4);        
        _logText->sciScintilla()->setBraceMatching(QsciScintilla::StrictBraceMatch);
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

    void OutputItemContentWidget::customEvent(QEvent *event)
    {
        return BaseClass::customEvent(event);
    }    

    void OutputItemContentWidget::refresh()
    {
        _shell->server()->query(0, _queryInfo);
    }

    void OutputItemContentWidget::onDeleteDocuments()
    {
        INotifier *observer = dynamic_cast<INotifier*>(sender());
        if(!observer)
            return;

        if (!_queryInfo._collectionInfo.isValid())
            return;

        QModelIndexList selectedIndexes = observer->selectedIndexes();
        if (!detail::isMultySelection(selectedIndexes))
            return;

        int answer = QMessageBox::question(dynamic_cast<QWidget*>(observer), "Delete", QString("Do you want to delete %1 selected documents?").arg(selectedIndexes.count()));
        if (answer == QMessageBox::Yes) {
            std::vector<BsonTreeItem*> items;
            for (QModelIndexList::const_iterator it = selectedIndexes.begin(); it!= selectedIndexes.end(); ++it) {
                BsonTreeItem *item = QtUtils::item<BsonTreeItem*>(*it);
                items.push_back(item);                
            }
            deleteDocuments(items,true);
        }
    }

    void OutputItemContentWidget::onDeleteDocument()
    {
        INotifier *observer = dynamic_cast<INotifier*>(sender());
        if(!observer)
            return;

        if (!_queryInfo._collectionInfo.isValid())
            return;

        QModelIndex selectedIndex = observer->selectedIndex();
        if (!selectedIndex.isValid())
            return;

        BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedIndex);
        std::vector<BsonTreeItem*> vec;
        vec.push_back(documentItem);
        return deleteDocuments(vec,false);
    }

    void OutputItemContentWidget::onEditDocument()
    {
        INotifier *observer = dynamic_cast<INotifier*>(sender());
        if(!observer)
            return;

        if (!_queryInfo._collectionInfo.isValid())
            return;

        QModelIndex selectedInd = observer->selectedIndex();
        if (!selectedInd.isValid())
            return;

        BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedInd);
        if (!documentItem)
            return;

        mongo::BSONObj obj = documentItem->superRoot();

        std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1,
            AppRegistry::instance().settingsManager()->uuidEncoding(),
            AppRegistry::instance().settingsManager()->timeZone());

        const QString &json = QtUtils::toQString(str);

        DocumentTextEditor editor(_queryInfo._collectionInfo,
            json, false, dynamic_cast<QWidget*>(observer));

        editor.setWindowTitle("Edit Document");
        int result = editor.exec();

        if (result == QDialog::Accepted) {
            _shell->server()->saveDocuments(editor.bsonObj(), _queryInfo._collectionInfo._ns);
        }
    }

    void OutputItemContentWidget::onViewDocument()
    {
        INotifier *observer = dynamic_cast<INotifier*>(sender());
        if(!observer)
            return;

        QModelIndex selectedIndex = observer->selectedIndex();
        if (!selectedIndex.isValid())
            return;

        BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedIndex);
        if (!documentItem)
            return;

        mongo::BSONObj obj = documentItem->superRoot();

        std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1,
            AppRegistry::instance().settingsManager()->uuidEncoding(),
            AppRegistry::instance().settingsManager()->timeZone());

        const QString &json = QtUtils::toQString(str);

        DocumentTextEditor *editor = new DocumentTextEditor(_queryInfo._collectionInfo,
            json, true, dynamic_cast<QWidget*>(observer));

        editor->setWindowTitle("View Document");
        editor->show();
    }

    void OutputItemContentWidget::onInsertDocument()
    {
        if (!_queryInfo._collectionInfo.isValid())
            return;

        DocumentTextEditor editor(_queryInfo._collectionInfo,
            "{\n    \n}", false, dynamic_cast<QWidget*>(parent()));

        editor.setCursorPosition(1, 4);
        editor.setWindowTitle("Insert Document");

        int result = editor.exec();
        if (result != QDialog::Accepted)
            return;

        DocumentTextEditor::ReturnType obj = editor.bsonObj();
        for (DocumentTextEditor::ReturnType::const_iterator it = obj.begin(); it != obj.end(); ++it) {
            _shell->server()->insertDocument(*it, _queryInfo._collectionInfo._ns);
        }

        refresh();
    }

    void OutputItemContentWidget::onCopyDocument()
    {
        INotifier *observer = dynamic_cast<INotifier*>(sender());
        if(!observer)
            return;

        QModelIndex selectedInd = observer->selectedIndex();
        if (!selectedInd.isValid())
            return;

        BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedInd);
        if (!documentItem)
            return;

        if (!detail::isSimpleType(documentItem))
            return;

        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(documentItem->value());
    }

    void OutputItemContentWidget::onCopyJson()
    {
        INotifier *observer = dynamic_cast<INotifier*>(sender());
        if(!observer)
            return;

        QModelIndex selectedInd = observer->selectedIndex();
        if (!selectedInd.isValid())
            return;

        BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedInd);
        if (!documentItem)
            return;

        if (!detail::isDocumentType(documentItem))
            return;

        QClipboard *clipboard = QApplication::clipboard();
        mongo::BSONObj obj = documentItem->root();
        if (documentItem != documentItem->superParent()){
            obj = obj[QtUtils::toStdString(documentItem->key())].Obj();
        }
        bool isArray = BsonUtils::isArray(documentItem->type());
        std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1,
            AppRegistry::instance().settingsManager()->uuidEncoding(),
            AppRegistry::instance().settingsManager()->timeZone(),isArray);

        const QString &json = QtUtils::toQString(str);
        clipboard->setText(json);
    }

    void OutputItemContentWidget::deleteDocuments(std::vector<BsonTreeItem*> items, bool force)
    {
        bool isNeededRefresh = false;
        for (std::vector<BsonTreeItem*>::const_iterator it = items.begin(); it != items.end(); ++it) {
            BsonTreeItem * documentItem = *it;
            if (!documentItem)
                break;

            mongo::BSONObj obj = documentItem->superRoot();
            mongo::BSONElement id = obj.getField("_id");

            if (id.eoo()) {
                QMessageBox::warning(dynamic_cast<QWidget*>(parent()), "Cannot delete", "Selected document doesn't have _id field. \n"
                    "Maybe this is a system document that should be managed in a special way?");
                break;
            }

            mongo::BSONObjBuilder builder;
            builder.append(id);
            mongo::BSONObj bsonQuery = builder.obj();
            mongo::Query query(bsonQuery);

            if (!force) {
                // Ask user
                int answer = utils::questionDialog(dynamic_cast<QWidget*>(parent()), "Delete",
                    "Document", "%1 %2 with id:<br><b>%3</b>?", QtUtils::toQString(id.toString(false)));

                if (answer != QMessageBox::Yes)
                    break;
            }

            isNeededRefresh=true;
            _shell->server()->removeDocuments(query, _queryInfo._collectionInfo._ns);
        }

        if (isNeededRefresh)
            refresh();
    }
}
