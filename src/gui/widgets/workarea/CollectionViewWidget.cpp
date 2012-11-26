#include "StdAfx.h"
#include "CollectionViewWidget.h"
#include "JsonTreeWidget.h"
#include "mongodb\bson\bsonobj.h"
#include "Mongo/MongoServiceOld.h"
#include <QStringBuilder>
#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexerjavascript.h>
#include "global.h"
#include "AppRegistry.h"

using namespace mongo;

//extern JSFunctionSpec theHelpers[];
//extern JSFunctionSpec theHelpers[];
//extern QList<BSONObj> _g_bsonObjects;
//extern QString _g_mongoLog;

CollectionViewWidget::CollectionViewWidget() : QWidget()
{
//	_shell = NULL;

	QStringList colums;
	colums << "Key" << "Value" << "Type";
	_jsonTree = new JsonTreeWidget(NULL);
	_jsonTree->setHeaderLabels(colums);
	_jsonTree->header()->setResizeMode(0, QHeaderView::Stretch);
	_jsonTree->header()->setResizeMode(1, QHeaderView::Stretch);
	_jsonTree->header()->setResizeMode(2, QHeaderView::Stretch);
	connect(_jsonTree, SIGNAL(itemExpanded(QTreeWidgetItem *)), this, SLOT(jsonItemExpanded(QTreeWidgetItem *)));

	_modeTabs = new QTabWidget;
	_modeTabs->setStyleSheet("JsonTreeWidget, QTextEdit { border: 0px solid gray;}");
	_modeTabs->addTab(_jsonTree, "Tree View");

	QsciLexerJavaScript * javaScriptLexer = new QsciLexerJavaScript;

	QsciLexerJavaScript * logLexer = new QsciLexerJavaScript;
	logLexer->setFont(QFont( "Courier New", 10));

	_resultText = new QsciScintilla;
	_resultText->setLexer(logLexer);
	_resultText->setWrapMode(QsciScintilla::WrapWord);
	_resultText->setVisible(false);
	
	_jsonText = new QsciScintilla;
	_jsonText->setLexer(javaScriptLexer);

	_logText = new QsciScintilla;
	_logText->setFixedHeight(200);
	_logText->setLexer(logLexer);
	_logText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_logText->setWrapMode(QsciScintilla::WrapWord);


	_modeTabs->addTab(_jsonText, "Text View");

	connect(_modeTabs, SIGNAL(currentChanged(int)), this, SLOT(tabPageChanged(int)));
	
	_queryText = new QsciScintilla;
	_queryText->setLexer(javaScriptLexer);
	_queryText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_queryText->setFixedHeight(23);
	_queryText->installEventFilter(this);

	connect(_queryText, SIGNAL(linesChanged()), this, SLOT(queryLinesCountChanged()));

	QPushButton * executeButton = new QPushButton("Execute");
	connect(executeButton, SIGNAL(clicked()), this, SLOT(execute()));

	QHBoxLayout * hlayout = new QHBoxLayout;
	hlayout->addWidget(_queryText, 0, Qt::AlignTop);
	hlayout->addWidget(executeButton, 0, Qt::AlignRight | Qt::AlignTop);;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addLayout(hlayout);
	layout->addWidget(_modeTabs, Qt::AlignJustify);
	layout->addWidget(_resultText, Qt::AlignJustify);
	layout->addWidget(_logText);
	setLayout(layout);
}

CollectionViewWidget::~CollectionViewWidget() { }

void CollectionViewWidget::displayObjects(QList<BSONObj> objs)
{
	_bsonObjects = objs;
	_jsonTree->setUpdatesEnabled(false);
	_jsonTree->clear();

	QList<QTreeWidgetItem *> items;
	foreach(BSONObj obj, objs)
	{
		JsonTreeWidgetItem * item = new JsonTreeWidgetItem(obj);
		item->setText(0, "{..}");
		item->setExpanded(true);
		item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
		items.append(item);
	}	

	_jsonTree->addTopLevelItems(items);
	_jsonTree->setUpdatesEnabled(true);
}

void CollectionViewWidget::populateObject(QTreeWidgetItem * item, BSONObj obj, bool inArray)
{
	BSONObjIterator i(obj);

	while (i.more())
	{
		BSONElement e = i.next();

		if (e.isSimpleType())
		{
			QString fieldName = QString::fromUtf8(e.fieldName());
			QString fieldValue = MongoServiceOld::getStringValue(e);
			
			QTreeWidgetItem * sitem = new QTreeWidgetItem;
			sitem->setText(0, fieldName);
			sitem->setText(1, fieldValue);
			sitem->setText(2, "Simple type");
			sitem->setExpanded(true);

			item->addChild(sitem);
		} 
		else if (e.isABSONObj())
		{
			QTreeWidgetItem * sitem = new QTreeWidgetItem;
			QString fieldName;

			if (inArray)
			{
				fieldName = QString("[%1] {..}").arg(QString::fromUtf8(e.fieldName()));
			}
			else if (e.type() == Array)
			{
				fieldName = QString("%1 [%2]").arg(QString::fromUtf8(e.fieldName())).arg(e.Array().size());
			}
			else
			{
				fieldName = QString("%1 {..}").arg(QString::fromUtf8(e.fieldName()));
			}

			sitem->setText(0, fieldName);
			sitem->setText(1, "");
			sitem->setText(2, "Object");
			sitem->setExpanded(true);
			item->addChild(sitem);

			populateObject(sitem, e.Obj(), e.type() == Array);
		}
	}
}

void CollectionViewWidget::jsonItemExpanded(QTreeWidgetItem * item)
{
 	JsonTreeWidgetItem * jsonItem = dynamic_cast<JsonTreeWidgetItem *>(item);
 
 	if (jsonItem)
 	{
		BSONObj obj = jsonItem->bsonObject();
		populateObject(item, obj);
 	}
}

void CollectionViewWidget::tabPageChanged(int index)
{
	if (index == 1)
	{
        _jsonText->setText("Loading...");

		TextPrepareThread * thread = new TextPrepareThread(_bsonObjects);

		connect(thread, SIGNAL(finished(QString)), this, SLOT(updateText(QString)));

		thread->start();
	}
}

void CollectionViewWidget::updateText(const QString & text)
{
	_jsonText->setUpdatesEnabled(false);
	_jsonText->setText(text);
	_jsonText->setUpdatesEnabled(true);
}

void CollectionViewWidget::queryLinesCountChanged()
{
	int h = _queryText->lines();
	_queryText->setFixedSize(_queryText->size().width(), h * 18 + 3);
}

void CollectionViewWidget::execute()
{
/*	if (!_shell)
	{
		_shell = new MongoShell(theHelpers);
		_shell->run();
    }*/

/*	_g_bsonObjects.clear();
	_g_mongoLog = "";
	_jsonTree->clear();

	QString t = _queryText->text();
	QByteArray byteArray = t.toUtf8();
	const char* cString = byteArray.constData();
    _shell->say(cString);

	if (_g_bsonObjects.count() > 0)
	{
		displayObjects(_g_bsonObjects);
		_resultText->setVisible(false);
		_modeTabs->setVisible(true);
	}
	else
	{
		_resultText->setVisible(true);
		_modeTabs->setVisible(false);
		_resultText->setText(_g_mongoLog);
	}

	int size = t.size() > 30 ? 30 : t.size();
	QString sub = t.mid(0, size);

	QString s = QString("\r\n> %1...").arg(sub);

	_logText->append(s);
	_logText->append(_g_mongoLog);

	QScrollBar *sb = _logText->verticalScrollBar();
	sb->setValue(sb->maximum());
    */
}

bool CollectionViewWidget::eventFilter(QObject * o, QEvent * e)
{
	if (e->type() == QEvent::KeyPress)
	{
		QKeyEvent * keyEvent = (QKeyEvent *) e;

		if ((keyEvent->modifiers() & Qt::ControlModifier) && (keyEvent->key()==Qt::Key_Return || keyEvent->key()==Qt::Key_Enter) ) 
		{
			execute();
			return true;
		}	
	}

	return false;
}

void CollectionViewWidget::setQuery(QString query)
{
	_queryText->setText(query);
}
