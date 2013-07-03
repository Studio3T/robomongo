#include "robomongo/gui/widgets/workarea/QueryWidget.h"

#include <QApplication>
#include <QtGui>
#include <QPlainTextEdit>
#include <QGraphicsScene>
#include <QGraphicsView>
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
namespace Robomongo
{
    QueryWidget::QueryWidget(MongoShell *shell, WorkAreaTabWidget *tabWidget, ViewMode viewMode, QWidget *parent) :
        QWidget(parent),
        _shell(shell),
        _tabWidget(tabWidget),
        _app(AppRegistry::instance().app()),
        _bus(AppRegistry::instance().bus()),
        _keyboard(AppRegistry::instance().keyboard()),
        _viewer(NULL),
        _viewMode(viewMode)
    {
        setObjectName("queryWidget");
        _bus->subscribe(this, DocumentListLoadedEvent::Type, shell);
        _bus->subscribe(this, ScriptExecutedEvent::Type, shell);
        _bus->subscribe(this, AutocompleteResponse::Type, shell);
        qDebug() << "Subscribed to ScriptExecutedEvent";

        _scriptWidget = new ScriptWidget(_shell);
        _scriptWidget->installEventFilter(this);

        _viewer = new OutputWidget(_viewMode, _shell);
        _outputLabel = new QLabel(this);
        _outputLabel->setContentsMargins(0, 5, 0, 0);
        _outputLabel->setVisible(false);
        _viewer->installEventFilter(this);

        QFrame *line = new QFrame(this);
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

		if (shell->execute()) {
			showProgress();
		}
    }

    void QueryWidget::execute()
    {
        QString query = _scriptWidget->selectedText();

        if (query.isEmpty())
            query = _scriptWidget->text();

        showProgress();
        _shell->open(query);
    }

    void QueryWidget::stop()
    {
        _shell->stop();
    }

    bool QueryWidget::eventFilter(QObject *o, QEvent *e)
    {
        if (e->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = (QKeyEvent *) e;
            if (_keyboard->isPreviousTabShortcut(keyEvent)) {
                _tabWidget->previousTab();
                return true;
            } else if (_keyboard->isNextTabShortcut(keyEvent)) {
                _tabWidget->nextTab();
                return true;
            } else if (_keyboard->isNewTabShortcut(keyEvent)) {
                openNewTab();
                return true;
            } else if (_keyboard->isSetFocusOnQueryLineShortcut(keyEvent)) {
                _scriptWidget->setScriptFocus();
                return true;
            } else if (_keyboard->isExecuteScriptShortcut(keyEvent)) {
                execute();
                return true;
            } else if (_keyboard->isAutoCompleteShortcut(keyEvent)) {
                _scriptWidget->showAutocompletion();
                return true;
            } else if (_keyboard->isHideAutoCompleteShortcut(keyEvent)) {
                _scriptWidget->hideAutocompletion();
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
        _bus->publish(new QueryWidgetUpdatedEvent(this, _currentResult.results().count()));
        _scriptWidget->setScriptFocus();
    }

    void QueryWidget::openNewTab()
    {
        MongoServer *server = _shell->server();
        QString query = _scriptWidget->selectedText();

        /*
        QString dbName = ""; //WAS: server->connectionRecord()->databaseName();
        if (_currentResults.count() > 0) {
            MongoShellResult lastResult = _currentResults.last();
            dbName = lastResult.databaseName;
        }
        */

        _app->openShell(server, query, _currentResult.currentDatabase());
    }
    void QueryWidget::saveToFile()
    {
        if(_shell){
            _shell->setScript(_scriptWidget->text());
            _shell->saveToFile();
        }
    }
    void QueryWidget::savebToFileAs()
    {
        if(_shell){
            _shell->setScript(_scriptWidget->text());
            _shell->saveToFileAs();
        }
    }
    void QueryWidget::openFile()
    {
        if(_shell){
            _shell->loadFromFile();
            _scriptWidget->setText(_shell->query());
        }
    }
    void QueryWidget::closeEvent(QCloseEvent *ev)
    {
        AppRegistry::instance().app()->closeShell(_shell);
        return BaseClass::closeEvent(ev);
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

    void QueryWidget::enterCustomMode()
    {
        if (_viewer)
            _viewer->enterCustomMode();
    }

    void QueryWidget::showProgress()
    {
        _viewer->showProgress();
    }

    void QueryWidget::hideProgress()
    {
        _viewer->hideProgress();
    }


    void QueryWidget::handle(DocumentListLoadedEvent *event)
    {
        hideProgress();
        _scriptWidget->hideProgress();
        _viewer->updatePart(event->resultIndex(), event->queryInfo(), event->documents()); // this should be in viewer, subscribed
                                                                                           // to ScriptExecutedEvent
    }

    void QueryWidget::handle(ScriptExecutedEvent *event)
    {
        hideProgress();
        _scriptWidget->hideProgress();
        _currentResult = event->result();
        _bus->publish(new QueryWidgetUpdatedEvent(this, _currentResult.results().count()));

        setUpdatesEnabled(false);
        updateCurrentTab();
        displayData(event->result().results(), event->empty());

        _scriptWidget->setup(event->result()); // this should be in ScriptWidget, which is subscribed to ScriptExecutedEvent
        _scriptWidget->setScriptFocus();       // and this

        setUpdatesEnabled(true);
    }

    void QueryWidget::handle(AutocompleteResponse *event)
    {
        _scriptWidget->showAutocompletion(event->list, event->prefix);
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
}
