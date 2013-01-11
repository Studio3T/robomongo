#include "robomongo/gui/widgets/workarea/QueryWidget.h"

#include <QApplication>
#include <QtGui>
#include <QPlainTextEdit>
#include "Qsci/qsciscintilla.h"
#include <Qsci/qscilexerjavascript.h>
#include <mongo/client/dbclient.h>
#include <mongo/bson/bsonobj.h>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/KeyboardManager.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/MongoShell.h"
#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/widgets/workarea/BsonWidget.h"
#include "robomongo/gui/widgets/workarea/OutputWidget.h"
#include "robomongo/gui/widgets/workarea/WorkAreaTabWidget.h"
#include "robomongo/gui/widgets/workarea/ScriptWidget.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/gui/editors/JSLexer.h"

using namespace mongo;
using namespace Robomongo;

/*
** Constructs query widget
*/
QueryWidget::QueryWidget(MongoShell *shell, WorkAreaTabWidget *tabWidget, const QString &script, bool textMode, QWidget *parent) :
    QWidget(parent),
    _shell(shell),
    _tabWidget(tabWidget),
    _app(AppRegistry::instance().app()),
    _bus(AppRegistry::instance().bus()),
    _keyboard(AppRegistry::instance().keyboard()),
    _viewer(NULL),
    _textMode(textMode),
    _initialized(false)
{
    setObjectName("queryWidget");

    _bus->subscribe(this, DocumentListLoadedEvent::Type, shell);
    _bus->subscribe(this, ScriptExecutedEvent::Type, shell);

    _scriptWidget = new ScriptWidget(_shell);
    _scriptWidget->setText(script);
    _scriptWidget->installEventFilter(this);

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
    _bsonWidget = new BsonWidget(NULL);
    _viewer = new OutputWidget(_textMode, _shell);
    _outputLabel = new QLabel();
    _outputLabel->setContentsMargins(0, 5, 0, 0);
    _outputLabel->setVisible(false);

//    _topStatusBar = new TopStatusBar(_shell);
//    _topStatusBar->setFrameShape(QFrame::StyledPanel);
//    _topStatusBar->setFrameShadow(QFrame::Raised);

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Raised);
    //line->setStyleSheet("margin-top: 2px;");

    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);
    //line2->setStyleSheet("margin-top: 1px;");

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(_scriptWidget, 0, Qt::AlignTop);
    layout->addWidget(line);
    layout->addWidget(_outputLabel, 0, Qt::AlignTop);
    //layout->addSpacing(2);
    layout->addWidget(_viewer, 1);
    setLayout(layout);
}

/*
** Destructs QueryWidget
*/
QueryWidget::~QueryWidget()
{
}

/*
** Execute query
*/
void QueryWidget::ui_executeButtonClicked()
{
    QString query = _scriptWidget->selectedText();

    if (query.isEmpty())
        query = _scriptWidget->text();

    _shell->open(query);
}

/*
** Override event filter
*/
bool QueryWidget::eventFilter(QObject * o, QEvent * e)
{
	if (e->type() == QEvent::KeyPress)
	{
		QKeyEvent * keyEvent = (QKeyEvent *) e;

        if (_keyboard->isNewTabShortcut(keyEvent)) {
            openNewTab();
            return true;
        }
        else if (_keyboard->isSetFocusOnQueryLineShortcut(keyEvent)) {
            _scriptWidget->setScriptFocus();
            return true;
        }
        else if (_keyboard->isExecuteScriptShortcut(keyEvent))
        {
            ui_executeButtonClicked();
            return true;
        }

	}

    return false;
}

void QueryWidget::toggleOrientation()
{
    _viewer->toggleOrientation();
}

void QueryWidget::activateTabContent()
{
    _scriptWidget->setScriptFocus();
}

void QueryWidget::openNewTab()
{
    MongoServer *server = _shell->server();
    QString query = _scriptWidget->selectedText();

    if (query.isEmpty())
        query = "";//_queryText->text();

    /*
    QString dbName = ""; //WAS: server->connectionRecord()->databaseName();
    if (_currentResults.count() > 0) {
        MongoShellResult lastResult = _currentResults.last();
        dbName = lastResult.databaseName;
    }
    */

    _app->openShell(server, query, "New Shell" /*dbName*/);
}

void QueryWidget::reload()
{
    ui_executeButtonClicked();
}

void QueryWidget::duplicate()
{
    _scriptWidget->selectAll();
    openNewTab();
}

void QueryWidget::enterTreeMode()
{
    if (_viewer)
        _viewer->enterTreeMode();
}

void QueryWidget::enterTextMode()
{
    if (_viewer)
        _viewer->enterTextMode();
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
void QueryWidget::vm_shellOutputRefreshed(const QString &shellOutput)
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
void QueryWidget::vm_queryUpdated(const QString &query)
{
    // _queryText->setText(query);
}

void QueryWidget::handle(DocumentListLoadedEvent *event)
{
    _viewer->updatePart(event->resultIndex, event->queryInfo, event->list);

/*    _queryText->setText(event->query);
    QList<MongoShellResult> list;
    list << MongoShellResult("", event->list, "", false);
    displayData(list);*/
}

void QueryWidget::handle(ScriptExecutedEvent *event)
{
    _currentResults = event->result.results;

    setUpdatesEnabled(false);
    int thisTab = _tabWidget->indexOf(this);

    if (thisTab != -1) {
        _tabWidget->setTabToolTip(thisTab, _shell->query());
        QString tabTitle = _shell->query()
                .left(41)
                .replace(QRegExp("[\n\r\t]"), " ");

        tabTitle = tabTitle.isEmpty() ? "New Shell" : tabTitle;

        _tabWidget->setTabText(thisTab, tabTitle);
    }

    if (_scriptWidget->text().isEmpty())
        _scriptWidget->setText(_shell->query());

    displayData(event->result.results, event->empty);
    _scriptWidget->statusBar()->setCurrentDatabase(event->result.currentDatabase, event->result.isCurrentDatabaseValid);
    _scriptWidget->statusBar()->setCurrentServer(event->result.currentServer, event->result.isCurrentServerValid);
    _scriptWidget->setScriptFocus();
    setUpdatesEnabled(true);
}

void QueryWidget::displayData(const QList<MongoShellResult> &results, bool empty)
{
    if (!empty) {
        if (results.count() == 0 && !_scriptWidget->text().isEmpty()) {
            _outputLabel->setText("  Script executed successfully, but there is no results to show.");
            _outputLabel->setVisible(true);
        } else {
            _outputLabel->setVisible(false);
        }
    }

    _viewer->present(results);
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
