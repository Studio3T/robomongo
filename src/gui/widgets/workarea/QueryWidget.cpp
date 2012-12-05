#include "QueryWidget.h"
//#include "BsonWidget.h"
#include "AppRegistry.h"
#include "mongo/client/dbclient.h"
#include "mongo/bson/bsonobj.h"
#include "domain/MongoCollection.h"
#include "domain/MongoDatabase.h"
#include "domain/MongoServer.h"
#include "domain/MongoShell.h"
#include <QApplication>
#include <QtGui>
#include "GuiRegistry.h"
#include "Dispatcher.h"
#include <QPlainTextEdit>
#include "Dispatcher.h"
#include "events/MongoEvents.h"
#include "BsonWidget.h"
#include "editors/PlainJavaScriptEditor.h"
#include "Qsci/qsciscintilla.h"
#include "Qsci/qscilexerjavascript.h"

using namespace mongo;
using namespace Robomongo;

/*
** Constructs query widget
*/
QueryWidget::QueryWidget(const MongoShellPtr &shell, QWidget *parent) :
    QWidget(parent),
    _shell(shell),
    _dispatcher(AppRegistry::instance().dispatcher())
{
    setObjectName("queryWidget");

    _dispatcher.subscribe(this, DocumentListLoadedEvent::EventType, shell.get());
    _dispatcher.subscribe(this, ScriptExecutedEvent::EventType, shell.get());

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
    _bsonWidget = new BsonWidget(this);

    QVBoxLayout * pageBoxLayout = new QVBoxLayout;
    pageBoxLayout->addSpacing(2);
    pageBoxLayout->addWidget(_pageSizeEdit, 0, Qt::AlignRight | Qt::AlignTop);

    QHBoxLayout * hlayout = new QHBoxLayout;
    hlayout->addWidget(_queryText, 0, Qt::AlignTop);
//    hlayout->addSpacing(5);
//    hlayout->addWidget(_leftButton, 0, Qt::AlignRight | Qt::AlignTop);
//    hlayout->addLayout(pageBoxLayout);
//    hlayout->addWidget(_rightButton, 0, Qt::AlignRight | Qt::AlignTop);
//    hlayout->addSpacing(5);
//    hlayout->addWidget(executeButton, 0, Qt::AlignRight | Qt::AlignTop);
//    hlayout->setSpacing(1);

    QVBoxLayout * layout = new QVBoxLayout;
    layout->setContentsMargins(0,3,3,0);
    layout->addLayout(hlayout);
    layout->addWidget(_bsonWidget);
    setLayout(layout);

    ui_queryLinesCountChanged();

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
//    QPainter painter = QPainter();
//    painter.drawLine(_queryText->geometry().topLeft(), _queryText->geometry().topRight());

    QFontMetrics m(_queryText->font());
    int lineHeight = m.lineSpacing() + 1;

    int numberOfLines = _queryText->lines();

    int height = numberOfLines * lineHeight + 8;

    int maxHeight = 18 * lineHeight + 8;
    if (height > maxHeight)
        height = maxHeight;

    _queryText->setFixedHeight(height);
}

/*
** Execute query
*/
void QueryWidget::ui_executeButtonClicked()
{
//    QString query = _queryText->text();

//    _shell->open(query);

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

bool QueryWidget::event(QEvent *event)
{
    R_HANDLE(event)
    R_EVENT(DocumentListLoadedEvent)
    R_EVENT(ScriptExecutedEvent)
    else return QWidget::event(event);
}

/*
** Configure QsciScintilla query widget
*/
void QueryWidget::_configureQueryText()
{
//    _queryText = new PlainJavaScriptEditor(this);
//    _queryText->setFixedHeight(28);
//    _queryText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    _queryText->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
//    _queryText->setFrameShape(JSEdit::NoFrame);
//    _queryText->setLineNumbersVisible(false);
//    _queryText->setCodeFoldingEnabled(false);
//    _queryText->setFixedHeight(200);

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

    QsciLexerJavaScript * javaScriptLexer = new QsciLexerJavaScript;
    javaScriptLexer->setFont(textFont);

    _queryText = new RoboScintilla;
    _queryText->setLexer(javaScriptLexer);
    _queryText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _queryText->setFixedHeight(23);
    _queryText->setAutoIndent(true);
    _queryText->setIndentationsUseTabs(false);
    _queryText->setIndentationWidth(4);
    _queryText->setUtf8(true);
    _queryText->installEventFilter(this);
    _queryText->setMarginWidth(1, 0); // to hide left gray column
    _queryText->setBraceMatching(QsciScintilla::StrictBraceMatch);
    _queryText->setFont(textFont);
    //_queryText->SendScintilla(QsciScintilla::SCI_SETFONTQUALITY, QsciScintilla::SC_EFF_QUALITY_LCD_OPTIMIZED);
//    _queryText->SendScintilla(QsciScintilla::SCI_STYLESETBOLD, 1);
//    _queryText->SendScintilla(QsciScintilla::SCI_STYLESETFONT, (unsigned long) 0, "courier");
//    _queryText->SendScintilla(QsciScintilla::SCI_STYLESETFONT, (unsigned long) 1, "courier");
//    _queryText->SendScintilla(QsciScintilla::SCI_STYLESETFONT, (unsigned long) 2, "courier");
//    _queryText->SendScintilla(QsciScintilla::SCI_STYLESETFONT, (unsigned long) 3, "courier");
//    _queryText->SendScintilla(QsciScintilla::SCI_STYLESETFONT, (unsigned long) 4, "courier");

    //textEdit->SendScintilla (QsciScintillaBase::SCI_SETKEYWORDS, "for if end");

    //_queryText->setFrameShape(QFrame::WinPanel);
    //_queryText->setFrameShadow(QFrame::Sunken);
    _queryText->setStyleSheet("QFrame {background-color: white; border: 1px solid #c7c5c4; border-radius: 4px; margin: 0px; padding: 0px;}");
    //_queryText->setStyleSheet("QFrame {border-radius: 4px;}");



    connect(_queryText, SIGNAL(linesChanged()), SLOT(ui_queryLinesCountChanged()));
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

void QueryWidget::handle(const DocumentListLoadedEvent *event)
{
    _bsonWidget->setDocuments(event->list);
}

void QueryWidget::handle(const ScriptExecutedEvent *event)
{
    _bsonWidget->setShellOutput(event->response);
//    QMessageBox::information(NULL, "Answer", event->response);
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
