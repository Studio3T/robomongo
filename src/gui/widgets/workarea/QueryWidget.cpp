#include "QueryWidget.h"
//#include "BsonWidget.h"
#include "AppRegistry.h"
#include "mongo/client/dbclient.h"
#include "mongo/bson/bsonobj.h"
#include "domain/MongoCollection.h"
#include "domain/MongoDatabase.h"
#include "domain/MongoServer.h"
#include <QApplication>
#include <QtGui>
#include "GuiRegistry.h"

using namespace mongo;
using namespace Robomongo;

/*
** Constructs query widget
*/
QueryWidget::QueryWidget(QWidget * parent) : QWidget(parent)
{
    // Query text widget
    _configureQueryText();

    // Execute button
    QPushButton * executeButton = new QPushButton("Execute");
	executeButton->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowRight));
    connect(executeButton, SIGNAL(clicked()), this, SLOT(ui_executeButtonClicked()));

    // Left button
    _leftButton = new QPushButton();
    _leftButton->setIcon(GuiRegistry::instance().leftIcon());
    _leftButton->setMaximumWidth(25);
    connect(_leftButton, SIGNAL(clicked()), SLOT(ui_leftButtonClicked()));

    // Right button
    _rightButton = new QPushButton();
    _rightButton->setIcon(GuiRegistry::instance().rightIcon());
    _rightButton->setMaximumWidth(25);
    connect(_rightButton, SIGNAL(clicked()), SLOT(ui_rightButtonClicked()));

    // Page size edit box
    _pageSizeEdit = new QLineEdit("100");
    _pageSizeEdit->setMaximumWidth(31);

    // Bson widget
    //_bsonWidget = new BsonWidget(this);

    QVBoxLayout * pageBoxLayout = new QVBoxLayout;
    pageBoxLayout->addSpacing(2);
    pageBoxLayout->addWidget(_pageSizeEdit, 0, Qt::AlignRight | Qt::AlignTop);

    QHBoxLayout * hlayout = new QHBoxLayout;
//	hlayout->addWidget(_queryText, 0, Qt::AlignTop);
    hlayout->addSpacing(5);
    hlayout->addWidget(_leftButton, 0, Qt::AlignRight | Qt::AlignTop);
    hlayout->addLayout(pageBoxLayout);
    //hlayout->addWidget(_pageSizeEdit, 0, Qt::AlignRight | Qt::AlignTop);
    hlayout->addWidget(_rightButton, 0, Qt::AlignRight | Qt::AlignTop);
    hlayout->addSpacing(5);
    hlayout->addWidget(executeButton, 0, Qt::AlignRight | Qt::AlignTop);
    hlayout->setSpacing(1);

//	QVBoxLayout * layout = new QVBoxLayout;
//	layout->addLayout(hlayout);
//	layout->addWidget(_bsonWidget);
//	setLayout(layout);

    // Connect to VM
//    connect(_viewModel, SIGNAL(documentsRefreshed(QList<MongoDocument_Pointer>)), SLOT(vm_documentsRefreshed(QList<MongoDocument_Pointer>)));
//    connect(_viewModel, SIGNAL(shellOutputRefreshed(QString)), SLOT(vm_shellOutputRefreshed(QString)));
//    connect(_viewModel, SIGNAL(pagingVisibilityChanged(bool)), SLOT(vm_pagingVisibilityChanged(bool)));
//    connect(_viewModel, SIGNAL(queryUpdated(QString)), SLOT(vm_queryUpdated(QString)));

//    _viewModel->done();
}

/*
** Destructs QueryWidget
*/
QueryWidget::~QueryWidget()
{
    // delete _viewModel;
}

/*
** Handle queryText linesCountChanged event
*/
void QueryWidget::ui_queryLinesCountChanged()
{
//    int h = _queryText->lines();
//    int height = h * 18 + 3;

//    if (height > 400)
//        height = 400;

//    _queryText->setFixedHeight(height);
}

/*
** Execute query
*/
void QueryWidget::ui_executeButtonClicked()
{
    //QString query = _queryText->selectedText();

//    if (query.isEmpty())
//        query = _queryText->text();

    //_viewModel->execute(query);
}

/*
** Override event filter
*/
bool QueryWidget::eventFilter(QObject * o, QEvent * e)
{
	if (e->type() == QEvent::KeyPress)
	{
		QKeyEvent * keyEvent = (QKeyEvent *) e;

		if ((keyEvent->modifiers() & Qt::ControlModifier) && (keyEvent->key()==Qt::Key_Return || keyEvent->key()==Qt::Key_Enter) ) 
		{
            ui_executeButtonClicked();
			return true;
		}	
	}

    return false;
}

/*
** Configure QsciScintilla query widget
*/
void QueryWidget::_configureQueryText()
{
//    QsciLexerJavaScript * javaScriptLexer = new QsciLexerJavaScript;
//    javaScriptLexer->setFont(QFont("Courier", 11));

//    _queryText = new QsciScintilla;
//    _queryText->setLexer(javaScriptLexer);
//    _queryText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    _queryText->setFixedHeight(23);
//    _queryText->setAutoIndent(true);
//    _queryText->setIndentationsUseTabs(false);
//    _queryText->setIndentationWidth(4);
//    _queryText->setUtf8(true);
//    _queryText->installEventFilter(this);
//    _queryText->setMarginWidth(1, 3); // to hide left gray column
//    _queryText->setBraceMatching(QsciScintilla::StrictBraceMatch);

//    connect(_queryText, SIGNAL(linesChanged()), SLOT(ui_queryLinesCountChanged()));
}

/*
** Documents refreshed
*/
void QueryWidget::vm_documentsRefreshed(const QList<MongoDocumentPtr> &documents)
{
    //_bsonWidget->setDocuments(documents);
}

/*
** Shell output refreshed
*/
void QueryWidget::vm_shellOutputRefreshed(const QString & shellOutput)
{
    //_bsonWidget->setShellOutput(shellOutput);
}

void QueryWidget::_showPaging(bool show)
{
    _leftButton->setVisible(show);
    _rightButton->setVisible(show);
}

/*
** Paging visability changed
*/
void QueryWidget::vm_pagingVisibilityChanged(bool show)
{
    _showPaging(show);
}

/*
** Query updated
*/
void QueryWidget::vm_queryUpdated(const QString & query)
{
   // _queryText->setText(query);
}

/*
** Paging right clicked
*/
void QueryWidget::ui_rightButtonClicked()
{
    int pageSize = _pageSizeEdit->text().toInt();
    if (pageSize == 0)
    {
        QMessageBox::information(NULL, "Page size incorrect", "Please specify correct page size");
        return;
    }

    //QString query = _queryText->text();

    //_viewModel->loadNextPage(query, pageSize);
}

/*
** Paging left clicked
*/
void QueryWidget::ui_leftButtonClicked()
{
    int pageSize = _pageSizeEdit->text().toInt();
    if (pageSize <= 0)
    {
        QMessageBox::information(NULL, "Page size incorrect", "Please specify correct page size");
        return;
    }

    //QString query = _queryText->text();
    //_viewModel->loadPreviousPage(query, pageSize);
}
