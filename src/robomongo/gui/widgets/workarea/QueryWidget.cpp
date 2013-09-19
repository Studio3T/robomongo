#include "robomongo/gui/widgets/workarea/QueryWidget.h"

#include <QApplication>
#include <QLabel>
#include <QFileInfo>
#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexerjavascript.h>
#include <mongo/client/dbclient.h>

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
#include "robomongo/core/utils/QtUtils.h"

#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/widgets/workarea/OutputWidget.h"
#include "robomongo/gui/widgets/workarea/WorkAreaTabWidget.h"
#include "robomongo/gui/widgets/workarea/ScriptWidget.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/gui/editors/JSLexer.h"

using namespace mongo;

namespace Robomongo
{
    QueryWidget::QueryWidget(MongoShell *shell, WorkAreaTabWidget *tabWidget, QWidget *parent) :
        QWidget(parent),
        _shell(shell),
        _tabWidget(tabWidget),
        _app(AppRegistry::instance().app()),
        _bus(AppRegistry::instance().bus()),
        _viewer(NULL),
        isTextChanged(false)
    {
        setObjectName("queryWidget");
        _bus->subscribe(this, DocumentListLoadedEvent::Type, shell);
        _bus->subscribe(this, ScriptExecutedEvent::Type, shell);
        _bus->subscribe(this, AutocompleteResponse::Type, shell);

        _scriptWidget = new ScriptWidget(_shell);
        VERIFY(connect(_scriptWidget,SIGNAL(textChanged()),this,SLOT(textChange())));
        _scriptWidget->installEventFilter(this);

        _viewer = new OutputWidget();
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

        if (shell->isExecutable()) {
            showProgress();
        }
    }

    void QueryWidget::execute()
    {
        QString query = _scriptWidget->selectedText();

        if (query.isEmpty())
            query = _scriptWidget->text();

        showProgress();
        _shell->open(QtUtils::toStdString(query));
    }

    void QueryWidget::stop()
    {
        _shell->stop();
    }

    bool QueryWidget::eventFilter(QObject *o, QEvent *e)
    {
        if (e->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = (QKeyEvent *) e;
            if (KeyboardManager::isPreviousTabShortcut(keyEvent)) {
                _tabWidget->previousTab();
                return true;
            } else if (KeyboardManager::isNextTabShortcut(keyEvent)) {
                _tabWidget->nextTab();
                return true;
            } else if (KeyboardManager::isNewTabShortcut(keyEvent)) {
                openNewTab();
                return true;
            } else if (KeyboardManager::isSetFocusOnQueryLineShortcut(keyEvent)) {
                _scriptWidget->setScriptFocus();
                return true;
            } else if (KeyboardManager::isExecuteScriptShortcut(keyEvent)) {
                execute();
                return true;
            } else if (KeyboardManager::isAutoCompleteShortcut(keyEvent)) {
                _scriptWidget->showAutocompletion();
                return true;
            } else if (KeyboardManager::isHideAutoCompleteShortcut(keyEvent)) {
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
        _bus->publish(new QueryWidgetUpdatedEvent(this, _currentResult.results().size()));
        _scriptWidget->setScriptFocus();
    }

    void QueryWidget::openNewTab()
    {
        MongoServer *server = _shell->server();
        QString query = _scriptWidget->selectedText();
        _app->openShell(server, query, _currentResult.currentDatabase());
    }

    void QueryWidget::saveToFile()
    {
        if(_shell){
            _shell->setScript(_scriptWidget->text());
            if(_shell->saveToFile()){
                isTextChanged =false;
                updateCurrentTab();
            }
        }
    }

    void QueryWidget::savebToFileAs()
    {
        if(_shell){
            _shell->setScript(_scriptWidget->text());
            if(_shell->saveToFileAs()){
                isTextChanged =false;
                updateCurrentTab();
            }
        }        
    }

    void QueryWidget::openFile()
    {
        if(_shell && _shell->loadFromFile()){
            _scriptWidget->setText(QtUtils::toQString(_shell->query()));
            isTextChanged = false;
            updateCurrentTab();
        }
    }

    void QueryWidget::textChange()
    {
        isTextChanged = true;
        updateCurrentTab();
    }

    QueryWidget::~QueryWidget()
    {
        AppRegistry::instance().app()->closeShell(_shell);
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

    void QueryWidget::enterTableMode()
    {
        if (_viewer)
            _viewer->enterTableMode();
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
        _viewer->updatePart(event->resultIndex(), event->queryInfo(), event->documents()); // this should be in viewer, subscribed to ScriptExecutedEvent
    }

    void QueryWidget::handle(ScriptExecutedEvent *event)
    {
        hideProgress();
        _scriptWidget->hideProgress();
        _currentResult = event->result();
        _bus->publish(new QueryWidgetUpdatedEvent(this, _currentResult.results().size()));

        setUpdatesEnabled(false);
        updateCurrentTab();
        displayData(event->result().results(), event->empty());

        _scriptWidget->setup(event->result()); // this should be in ScriptWidget, which is subscribed to ScriptExecutedEvent
        _scriptWidget->setScriptFocus();       // and this

        setUpdatesEnabled(true);
    }

    void QueryWidget::handle(AutocompleteResponse *event)
    {
        _scriptWidget->showAutocompletion(event->list, QtUtils::toQString(event->prefix) );
    }

    void QueryWidget::updateCurrentTab()  // !!!!!!!!!!! this method should be in
                                          // WorkAreaTabWidget, subscribed to ScriptExecutedEvent
    {
        int thisTab = _tabWidget->indexOf(this);

        if (thisTab != -1) {
            const QString &shellQuery = QtUtils::toQString(_shell->query());
            QString toolTipQuery = shellQuery.left(700);

            QString tabTitle,toolTipText;
            if(_shell){
                QFileInfo fileInfo(_shell->filePath());
                if(fileInfo.isFile()){
                     tabTitle = fileInfo.fileName();
                     toolTipText = fileInfo.filePath();
                }
            }

            if(tabTitle.isEmpty()&&shellQuery.isEmpty()){
                tabTitle = "New Shell";
            }
            else{

                if(tabTitle.isEmpty()){
                    tabTitle = shellQuery.left(41).replace(QRegExp("[\n\r\t]"), " ");;
                    toolTipText = QString("<pre>%1</pre>").arg(toolTipQuery);
                }
                else{
                    //tabTitle = QString("%1 %2").arg(tabTitle).arg(shellQuery);
                    toolTipText = QString("<b>%1</b><br/><pre>%2</pre>").arg(toolTipText).arg(toolTipQuery);
                }
            }

            if(isTextChanged){
                tabTitle = "* " + tabTitle;
            }
 
            _tabWidget->setTabText(thisTab, tabTitle);
            _tabWidget->setTabToolTip(thisTab, toolTipText);
        }
    }

    void QueryWidget::displayData(const std::vector<MongoShellResult> &results, bool empty)
    {
        if (!empty) {
            if (results.size() == 0 && !_scriptWidget->text().isEmpty()) {
                _outputLabel->setText("  Script executed successfully, but there is no results to show.");
                _outputLabel->setVisible(true);
            } else {
                _outputLabel->setVisible(false);
            }
        }

        _viewer->present(_shell,results);
    }
}
