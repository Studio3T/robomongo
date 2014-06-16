#include "robomongo/gui/widgets/workarea/QueryWidget.h"

#include <QApplication>
#include <QLabel>
#include <QFileInfo>
#include <QVBoxLayout>
#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexerjavascript.h>
#include <mongo/client/dbclient.h>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/MongoShell.h"
#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/utils/QtUtils.h"

#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/widgets/workarea/OutputWidget.h"
#include "robomongo/gui/widgets/workarea/ScriptWidget.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/gui/editors/JSLexer.h"

using namespace mongo;

namespace Robomongo
{
    QueryWidget::QueryWidget(MongoShell *shell, QWidget *parent) :
        QWidget(parent),
        _shell(shell),
        _viewer(NULL),
        _isTextChanged(false)
    {
        AppRegistry::instance().bus()->subscribe(this, DocumentListLoadedEvent::Type, shell);
        AppRegistry::instance().bus()->subscribe(this, ScriptExecutedEvent::Type, shell);
        AppRegistry::instance().bus()->subscribe(this, AutocompleteResponse::Type, shell);

        _scriptWidget = new ScriptWidget(_shell);
        VERIFY(connect(_scriptWidget,SIGNAL(textChanged()),this,SLOT(textChange())));

        _viewer = new OutputWidget();
        _outputLabel = new QLabel(this);
        _outputLabel->setContentsMargins(0, 5, 0, 0);
        _outputLabel->setVisible(false);

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
    }

    void QueryWidget::setScriptFocus()
    {
        _scriptWidget->setScriptFocus();
    }

    void QueryWidget::showAutocompletion()
    {
        _scriptWidget->showAutocompletion();
    }

    void QueryWidget::hideAutocompletion()
    {
        _scriptWidget->hideAutocompletion();
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

    void QueryWidget::toggleOrientation()
    {
        _viewer->toggleOrientation();
    }

    void QueryWidget::openNewTab()
    {
        if(_shell){
            MongoServer *server = _shell->server();
            QString query = _scriptWidget->selectedText();
            AppRegistry::instance().app()->openShell(server, query, _currentResult.currentDatabase(), AppRegistry::instance().settingsManager()->autoExec());
        }
    }

    void QueryWidget::saveToFile()
    {
        if (_shell) {
            _shell->setScript(_scriptWidget->text());
            if (_shell->saveToFile()) {
                _isTextChanged =false;
                updateCurrentTab();
            }
        }
    }

    void QueryWidget::savebToFileAs()
    {
        if (_shell) {
            _shell->setScript(_scriptWidget->text());
            if (_shell->saveToFileAs()) {
                _isTextChanged =false;
                updateCurrentTab();
            }
        }        
    }

    void QueryWidget::openFile()
    {
        if (_shell && _shell->loadFromFile()) {
            _scriptWidget->setText(QtUtils::toQString(_shell->query()));
            _isTextChanged = false;
            updateCurrentTab();
        }
    }

    void QueryWidget::textChange()
    {
        _isTextChanged = true;
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
        _viewer->enterTreeMode();
    }

    void QueryWidget::enterTextMode()
    {
        _viewer->enterTextMode();
    }

    void QueryWidget::enterTableMode()
    {
        _viewer->enterTableMode();
    }

    void QueryWidget::enterCustomMode()
    {
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
        _viewer->updatePart(event->resultIndex(), event->queryInfo(), event->documents()); // this should be in viewer, subscribed to ScriptExecutedEvent
    }

    void QueryWidget::handle(ScriptExecutedEvent *event)
    {
        hideProgress();
        _currentResult = event->result();        

        updateCurrentTab();
        displayData(event->result().results(), event->empty());
        _scriptWidget->setup(event->result()); // this should be in ScriptWidget, which is subscribed to ScriptExecutedEvent              
        activateTabContent();
    }

    void QueryWidget::activateTabContent()
    {
        AppRegistry::instance().bus()->publish(new QueryWidgetUpdatedEvent(this, _currentResult.results().size()));
        _scriptWidget->setScriptFocus();
    }

    void QueryWidget::handle(AutocompleteResponse *event)
    {
        _scriptWidget->showAutocompletion(event->list, QtUtils::toQString(event->prefix) );
    }

    void QueryWidget::updateCurrentTab()
    {
        const QString &shellQuery = QtUtils::toQString(_shell->query());
        QString toolTipQuery = shellQuery.left(700);

        QString tabTitle,toolTipText;
        if (_shell) {
            QFileInfo fileInfo(_shell->filePath());
            if (fileInfo.isFile()) {
                    tabTitle = fileInfo.fileName();
                    toolTipText = fileInfo.filePath();
            }
        }

        if (tabTitle.isEmpty()&&shellQuery.isEmpty()) {
            tabTitle = "New Shell";
        }
        else {

            if (tabTitle.isEmpty()) {
                tabTitle = shellQuery.left(41).replace(QRegExp("[\n\r\t]"), " ");;
                toolTipText = QString("<pre>%1</pre>").arg(toolTipQuery);
            }
            else {
                //tabTitle = QString("%1 %2").arg(tabTitle).arg(shellQuery);
                toolTipText = QString("<b>%1</b><br/><pre>%2</pre>").arg(toolTipText).arg(toolTipQuery);
            }
        }

        if (_isTextChanged) {
            tabTitle = "* " + tabTitle;
        }

        emit titleChanged(tabTitle);
        emit toolTipChanged(toolTipText);
    }

    void QueryWidget::displayData(const std::vector<MongoShellResult> &results, bool empty)
    {
        if (!empty) {
            bool isOutVisible = results.size() == 0 && !_scriptWidget->text().isEmpty();
            if (isOutVisible) {
                _outputLabel->setText("  Script executed successfully, but there are no results to show.");
            }
            _outputLabel->setVisible(isOutVisible);
        }

        _viewer->present(_shell,results);
    }
}
