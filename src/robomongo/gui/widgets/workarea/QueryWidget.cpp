#include "robomongo/gui/widgets/workarea/QueryWidget.h"

#include <QApplication>
#include <QtGui>
#include <QPlainTextEdit>
#include <Qsci/qsciscintilla.h>
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

QueryWidget::QueryWidget(MongoShell *shell, WorkAreaTabWidget *tabWidget,
                         const ScriptInfo &scriptInfo, bool textMode, QWidget *parent) :
    QWidget(parent),
    _shell(shell),
    _tabWidget(tabWidget),
    _app(AppRegistry::instance().app()),
    _bus(AppRegistry::instance().bus()),
    _keyboard(AppRegistry::instance().keyboard()),
    _viewer(NULL),
    _textMode(textMode)
{
    setObjectName("queryWidget");
    _bus->subscribe(this, DocumentListLoadedEvent::Type, shell);
    _bus->subscribe(this, ScriptExecutedEvent::Type, shell);

    _scriptWidget = new ScriptWidget(_shell);
    _scriptWidget->setText(scriptInfo.script());
    _scriptWidget->setTextCursor(scriptInfo.cursor());
    _scriptWidget->installEventFilter(this);

    _viewer = new OutputWidget(_textMode, _shell);
    _outputLabel = new QLabel();
    _outputLabel->setContentsMargins(0, 5, 0, 0);
    _outputLabel->setVisible(false);

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Raised);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(_scriptWidget, 0, Qt::AlignTop);
    layout->addWidget(line);
    layout->addWidget(_outputLabel, 0, Qt::AlignTop);
    layout->addWidget(_viewer, 1);
    setLayout(layout);
}

void QueryWidget::execute()
{
    QString query = _scriptWidget->selectedText();

    if (query.isEmpty())
        query = _scriptWidget->text();

    _shell->open(query);
}

bool QueryWidget::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = (QKeyEvent *) e;
        if (_keyboard->isNewTabShortcut(keyEvent)) {
            openNewTab();
            return true;
        } else if (_keyboard->isSetFocusOnQueryLineShortcut(keyEvent)) {
            _scriptWidget->setScriptFocus();
            return true;
        } else if (_keyboard->isExecuteScriptShortcut(keyEvent)) {
            execute();
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
    execute();
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

void QueryWidget::handle(DocumentListLoadedEvent *event)
{
    _viewer->updatePart(event->resultIndex(), event->queryInfo(), event->documents()); // this should be in viewer, subscribed
                                                                                       // to ScriptExecutedEvent
}

void QueryWidget::handle(ScriptExecutedEvent *event)
{
    _currentResults = event->result().results();

    setUpdatesEnabled(false);
    updateCurrentTab();
    displayData(event->result().results(), event->empty());

    _scriptWidget->setup(event->result()); // this should be in ScriptWidget, which is subscribed to ScriptExecutedEvent
    _scriptWidget->setScriptFocus();       // and this

    setUpdatesEnabled(true);
}

QString QueryWidget::buildTabTitle(const QString &query)
{
    QString tabTitle = query
            .left(41)
            .replace(QRegExp("[\n\r\t]"), " ");

    tabTitle = tabTitle.isEmpty() ? "New Shell" : tabTitle;
    return tabTitle;
}

void QueryWidget::updateCurrentTab()  // !!!!!!!!!!! this method should be in
                                      // WorkAreaTabWidget, subscribed to ScriptExecutedEvent
{
    int thisTab = _tabWidget->indexOf(this);

    if (thisTab != -1) {
        _tabWidget->setTabToolTip(thisTab, _shell->query());
        _tabWidget->setTabText(thisTab, buildTabTitle(_shell->query()));
    }
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
