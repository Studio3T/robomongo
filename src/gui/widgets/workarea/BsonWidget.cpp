#include "BsonWidget.h"
#include "BsonTreeWidget.h"
#include "domain/MongoDocumentIterator.h"
#include "domain/MongoElement.h"
#include "domain/MongoDocument.h"
#include <QPlainTextEdit>
#include <QStackedWidget>

using namespace Robomongo;

BsonWidget::BsonWidget(QWidget * parent) : QWidget(parent)
{
    _textRendered = false;
	_modeTabs = new QTabWidget;
    _modeTabs->setTabPosition(QTabWidget::South);
    _modeTabs->setDocumentMode(true);
    _bsonTree = new BsonTreeWidget;


//    _jsonText = new QPlainTextEdit(this);
//    _logText = new QPlainTextEdit(this);

//	QsciLexerJavaScript * javaScriptLexer = new QsciLexerJavaScript;
//	javaScriptLexer->setFont(QFont( "Courier New", 10));

//	_jsonText = new QsciScintilla;
//	_jsonText->setAutoIndent(true);
//	_jsonText->setIndentationsUseTabs(false);
//	_jsonText->setIndentationWidth(4);
//	_jsonText->setUtf8(true);
//	_jsonText->setLexer(javaScriptLexer);
//	_jsonText->setFolding(QsciScintilla::BoxedFoldStyle);
//	_jsonText->setEdgeMode(QsciScintilla::EdgeNone);
//	_jsonText->setMarginWidth(1, 0);

//    _logText = new QsciScintilla;
//    _logText->setUtf8(true);
//    _logText->setLexer(javaScriptLexer);
//    _logText->setFolding(QsciScintilla::BoxedFoldStyle);
//    _logText->setEdgeMode(QsciScintilla::EdgeNone);
//    _logText->setMarginWidth(1, 0);


/*	_modeTabs->addTab(_bsonTree, "Tree");
    _modeTabs->addTab(_jsonText, "Text");
    _modeTabs->setStyleSheet(
        "BsonTreeWidget, QsciScintilla { border: 0px;} "
    );*/

    //_modeTabs->setDocumentMode(true);
    //connect(_modeTabs, SIGNAL(currentChanged(int)), this, SLOT(ui_tabPageChanged(int)));

	QHBoxLayout * hlayout = new QHBoxLayout;
    hlayout->setSpacing(0);
	hlayout->setMargin(0);
    hlayout->addWidget(_bsonTree);

	setLayout(hlayout);
}

BsonWidget::~BsonWidget()
{

}

void BsonWidget::setDocuments(const QList<MongoDocumentPtr> & documents)
{
    // Reset flag
    _textRendered = false;

	_documents = documents;
	_bsonTree->setDocuments(documents);

    if (!_documents.isEmpty())
        _modeTabs->setCurrentIndex(0);
}

/*
** Handle mode tabs (Tree/Text) current page index changed event
*/ 
void BsonWidget::ui_tabPageChanged(int index)
{
//	// If text mode
//	if (index == 1)
//	{
//        if (_textRendered)
//            return;

//		_jsonText->setText("Loading...");
//		JsonPrepareThread * thread = new JsonPrepareThread(_documents);
//		connect(thread, SIGNAL(finished(QString)), SLOT(thread_jsonPrepared(QString)));
//		thread->start();
//	}

//    if (index == 2)
//    {
//        _modeTabs->setTabText(2, "Shell");
//    }
}

/*
** Handle moment when json prepared
*/
void BsonWidget::thread_jsonPrepared(const QString & json)
{
//	_jsonText->setUpdatesEnabled(false);
//	_jsonText->setText(json);
//    _jsonText->setUpdatesEnabled(true);
//    _textRendered = true;
}

void BsonWidget::setShellOutput(const QString & output)
{
/*    _modeTabs->clear();
    _modeTabs->addTab(_logText, "Shell");
*/
/*    _logText->setPlainText(output);

    if (!output.isEmpty() && _documents.count() == 0)
        _modeTabs->setCurrentIndex(2);
    else if (!output.isEmpty())
    {
        _modeTabs->setTabText(2, "Shell *");
    }*/
}
