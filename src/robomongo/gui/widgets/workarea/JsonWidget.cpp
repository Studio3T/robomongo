#include "StdAfx.h"
#include "JsonWidget.h"
#include "Mongo/MongoServiceOld.h"
#include <QSize>
#include <QSizePolicy.h>
#include <Qsci\qscilexerjavascript.h>

JsonWidget::JsonWidget(BSONObj bsonObject, QWidget * parent) : QWidget(parent)
{
	_bsonObject = bsonObject;
	_initialized = false;
	_customData = NULL;

//	setFixedSize(900, 500);

	collapse();

	createToggleButton();

	// Testing
//	setGeometry(300, 300, 900, 500);
}

JsonWidget::~JsonWidget() { }

/*
** Populate tree with BSONObject 
** If item is null we are adding top level item.
*/
void JsonWidget::populateObject(QTreeWidgetItem * item, BSONObj obj, bool inArray)
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

			if (item)
				item->addChild(sitem);
			else
				_treeWidget->addTopLevelItem(sitem);
		} 
		else if (e.isABSONObj())
		{
			QTreeWidgetItem * sitem = new QTreeWidgetItem;
			QString fieldName;

/*			if (inArray)
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
            }*/

			sitem->setText(0, fieldName);
			sitem->setText(1, "");
			sitem->setText(2, "Object");
			sitem->setExpanded(true);

			if (item)
				item->addChild(sitem);
			else
				_treeWidget->addTopLevelItem(sitem);

			populateObject(sitem, e.Obj(), e.type() == Array);
		}
	}
}

/*
** Create and configure tree widget 
*/
void JsonWidget::createTreeWidget()
{
	_treeWidget = new QTreeWidget(this);
	connect(_treeWidget, SIGNAL(itemExpanded(QTreeWidgetItem *)), this, SLOT(treeItemExpanded(QTreeWidgetItem *)));
	connect(_treeWidget, SIGNAL(itemCollapsed(QTreeWidgetItem *)), this, SLOT(treeItemCollapsed(QTreeWidgetItem *)));

	QStringList colums;
	colums << "Key" << "Value" << "Type";
	_treeWidget->setHeaderLabels(colums);
	_treeWidget->header()->setResizeMode(0, QHeaderView::Stretch);
	_treeWidget->header()->setResizeMode(1, QHeaderView::Stretch);
	_treeWidget->header()->setResizeMode(2, QHeaderView::Stretch);
}

/*
** Create and configure mode tab widget
*/
void JsonWidget::createModeTabWidget()
{
	_modeTabWidget = new QTabWidget;
	_modeTabWidget->setStyleSheet("QTabWidget::tab-bar { alignment: right; }");
	_modeTabWidget->addTab(_treeWidget, "Tree");
	_modeTabWidget->addTab(_textEdit, "Text");
	connect(_modeTabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabPageChanged(int)));
}

/*
** Create and configure text edit 
*/
void JsonWidget::createTextEdit()
{
	_textEdit = new QsciScintilla;

	QsciLexerJavaScript * lexer = new QsciLexerJavaScript;
	_textEdit->setLexer(lexer);
}

/*
** Collapse widget
** In collapsed mode only one line of widget is visible. 
*/
void JsonWidget::collapse()
{
	// save height for future expanding
	_height = size().height();
	setFixedSize(size().width(), COLLAPSED_HEIGHT);

	if (_initialized)
	{
		// tabs are hidden in collapsed state
		_modeTabWidget->setVisible(false);
	}

	emit geometryChanged(this, size());
}

/*
** Expand widget
** In expanded mode widget is fully available
*/
void JsonWidget::expand()
{
	if (!_initialized)
	{
		// Create widgets. Order of calls is important!
		createTreeWidget();
		createTextEdit();
		createModeTabWidget();
		createLayout();

		_toggleButton->raise();
		_initialized = true;

		populateObject(NULL, _bsonObject, false);
	}

	resizeToContent();
//	setFixedSize(size().width(), _height);

	// tabs are visible in expanded state
	_modeTabWidget->setVisible(true);

	emit geometryChanged(this, size());
}

/*
** Toggle between expanded and collapsed mode
*/
void JsonWidget::toggle()
{
	if (size().height() > COLLAPSED_HEIGHT)
		collapse();
	else 
		expand();
}

/*
** Create and configure toggle button
*/
void JsonWidget::createToggleButton()
{
	_toggleButton = new QPushButton(" > ", this);
	_toggleButton->setGeometry(9, 7, 30, 20);
	connect(_toggleButton, SIGNAL(clicked()), this, SLOT(toggle()));
}

/*
** Create and configure layout of widget
*/
void JsonWidget::createLayout()
{
	QVBoxLayout * layout = new QVBoxLayout;
	//	layout->addLayout(hlayout);
	layout->addWidget(_modeTabWidget, Qt::AlignJustify);
	setLayout(layout);
}

/*
** Fires when tab page is changed(from Tree mode to Text mode)
*/
void JsonWidget::tabPageChanged(int index)
{
	if (index == 0)
	{
		resizeToContent();
	} 
	else if (index == 1)
	{
		string jsonSt = _bsonObject.jsonString(TenGen, 1);
		QString st = QString::fromStdString(jsonSt);

		_textEdit->setText(st);
		_textEdit->lines();
		int h = _textEdit->lines();
		setFixedSize(size().width(), 60 + h * 20 + 3);
	}

	emit geometryChanged(this, size());
	//_listItem->setSizeHint(QSize(size().width(), size().height()));
}

/*
** Fires when tree item expanded by user
*/
void JsonWidget::treeItemExpanded(QTreeWidgetItem * item)
{
	resizeToContent();
}

/*
** Fires when tree item collapsed by user
*/
void JsonWidget::treeItemCollapsed(QTreeWidgetItem * item)
{
	resizeToContent();
}

void JsonWidget::resizeToContent()
{
	int rows = 0;
	int itemHeight = 0;

	for (int i = 0; i < _treeWidget->topLevelItemCount(); i++)
	{
		QTreeWidgetItem * item =  _treeWidget->topLevelItem (i);
		
		rows++;
		if (item->isExpanded())
			rows += getNumberOfExpandedItems(item);
	}

	int height = 60 + rows * 20;

	int width = size().width();

	if (parentWidget())
		width = parentWidget()->size().width();

	setFixedSize(width, height);
	emit geometryChanged(this, size());

//	QMessageBox::information(this, "Hello", QString::number(h));
}

int JsonWidget::getNumberOfExpandedItems(QTreeWidgetItem * item)
{
	int count = 0;
	for (int i = 0; i < item->childCount(); i++)
	{
		QTreeWidgetItem * child =  item->child(i);
		count++;

		if (item->isExpanded())
			count += getNumberOfExpandedItems(child);
	}

	return count;
}
