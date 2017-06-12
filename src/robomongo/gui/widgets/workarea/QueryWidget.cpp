#include "robomongo/gui/widgets/workarea/QueryWidget.h"

#include <QObject>
#include <QPushButton>
#include <QApplication>
#include <QLabel>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QMainWindow>
#include <QDockWidget>
#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexerjavascript.h>
#include <mongo/client/dbclientinterface.h>

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
#include "robomongo/core/utils/Logger.h"

#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/widgets/workarea/OutputWidget.h"
#include "robomongo/gui/widgets/workarea/ScriptWidget.h"
#include "robomongo/gui/widgets/workarea/OutputItemContentWidget.h"
#include "robomongo/gui/widgets/workarea/OutputItemHeaderWidget.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/gui/editors/JSLexer.h"
#include "robomongo/gui/dialogs/ChangeShellTimeoutDialog.h"

using namespace mongo;

namespace Robomongo
{
    QueryWidget::QueryWidget(MongoShell *shell, QWidget *parent) :
        QWidget(parent),
        _shell(shell),
        _viewer(nullptr),
        _dock(nullptr),
        _isTextChanged(false)
    {
        AppRegistry::instance().bus()->subscribe(this, DocumentListLoadedEvent::Type, shell);
        AppRegistry::instance().bus()->subscribe(this, ScriptExecutedEvent::Type, shell);
        AppRegistry::instance().bus()->subscribe(this, AutocompleteResponse::Type, shell);

        // Make QMessageBox text selectable
        // setStyleSheet("QMessageBox { messagebox-text-interaction-flags: 5; }");

        _scriptWidget = new ScriptWidget(_shell, this);
        VERIFY(connect(_scriptWidget, SIGNAL(textChanged()), this, SLOT(textChange())));

        // Need to use QMainWindow in order to make use of all features of docking.
        // (Note: Qt full support for dock windows implemented only for QMainWindow)
        _viewer = new OutputWidget(this);
        _outputWindow = new QMainWindow;
        _dock = new CustomDockWidget(this);
        _dock->setAllowedAreas(Qt::NoDockWidgetArea);
        _dock->setFeatures(QDockWidget::DockWidgetFloatable);
        _dock->setWidget(_viewer);
        _dock->setTitleBarWidget(new QWidget);
        VERIFY(connect(_dock, SIGNAL(topLevelChanged(bool)), this, SLOT(on_dock_undock())));
        _outputWindow->addDockWidget(Qt::BottomDockWidgetArea, _dock);

        _outputLabel = new QLabel(this);
        _outputLabel->setContentsMargins(0, 5, 0, 0);
        _outputLabel->setVisible(false);

        _line = new QFrame(this);
        _line->setFrameShape(QFrame::HLine);
        _line->setFrameShadow(QFrame::Raised);

        _mainLayout = new QVBoxLayout;
        _mainLayout->setSpacing(0);
        _mainLayout->setContentsMargins(0, 0, 0, 0);
        _mainLayout->addWidget(_scriptWidget); 
        _mainLayout->addWidget(_line);
        _mainLayout->addWidget(_outputLabel, 0, Qt::AlignTop);
        _mainLayout->addWidget(_outputWindow, 1);      
        setLayout(_mainLayout);
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

    void QueryWidget::bringDockToFront()
    {
        _dock->raise(); // required for MAC only; possible Qt bug
        _dock->activateWindow();
    }

    bool QueryWidget::outputWindowDocked() const
    {
        if (_dock) {
            return !_dock->isFloating();
        }
        else {  // _dock is not initialized yet, but it will be docked when initialized
            return true;
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

    void QueryWidget::toggleOrientation()
    {
        _viewer->toggleOrientation();
    }

    void QueryWidget::openNewTab()
    {
        if (_shell) {
            MongoServer *server = _shell->server();
            QString query = _scriptWidget->selectedText();
            AppRegistry::instance().app()->openShell(server, query, _currentResult.currentDatabase(), 
                AppRegistry::instance().settingsManager()->autoExec());
        }
    }

    void QueryWidget::saveToFile()
    {
        if (_shell) {
            _shell->setScript(_scriptWidget->text());
            if (_shell->saveToFile()) {
                _isTextChanged = false;
                updateCurrentTab();
            }
        }
    }

    void QueryWidget::savebToFileAs()
    {
        if (_shell) {
            _shell->setScript(_scriptWidget->text());
            if (_shell->saveToFileAs()) {
                _isTextChanged = false;
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

    void QueryWidget::dockUndock() 
    {
        // Toggle between dock/undock
        _dock->setFloating(!_dock->isFloating());
    };
    
    void QueryWidget::changeShellTimeout() 
    {
        changeShellTimeoutDialog();
    }

    void QueryWidget::hideProgress()
    {
        _viewer->hideProgress();
    }

    void QueryWidget::handle(DocumentListLoadedEvent *event)
    {
        hideProgress();

        if (event->isError()) {
            QString message = QString("Failed to load documents.\n\nError:\n%1")
                .arg(QtUtils::toQString(event->error().errorMessage()));
            QMessageBox::information(this, "Error", message);
            return;
        }

        _viewer->updatePart(event->resultIndex(), event->queryInfo(), event->documents()); // this should be in viewer, subscribed to ScriptExecutedEvent
    }

    void QueryWidget::handle(ScriptExecutedEvent *event)
    {
        hideProgress();

        _currentResult = event->result();

        updateCurrentTab();
        displayData(event->result().results(), event->empty());
        // this should be in ScriptWidget, which is subscribed to ScriptExecutedEvent              
        _scriptWidget->setup(event->result()); 
        activateTabContent();

        if (event->isError()) {
            // For some cases, event error message already contains string "Error:"
            QString const& subStr =
                QString::fromStdString(event->error().errorMessage()).startsWith("Error", Qt::CaseInsensitive) ?
                "" : "Error:\n";

            QString const& message = "Failed to execute script.\n\n" + subStr +
                QString::fromStdString((event->error().errorMessage()));

            QMessageBox::critical(this, "Error", message);
        }

        if (event->timeoutReached()) {
            auto const shellTimeoutSec = AppRegistry::instance().settingsManager()->shellTimeoutSec();
            QString const subStr = _currentResult.results().size() > 1 ?
                "At least one of the scripts has reached shell timeout" :
                "The script has reached shell timeout";
            QString const secondStr = (shellTimeoutSec > 1) ? " seconds)" : " second)";
            QString messageShort = "Failed to execute all of the script. " + subStr + " (" +
                                    QString::number(shellTimeoutSec) + secondStr + " limit. ";
            QString messageLong = messageShort + 
                                  "\n\nPlease increase the value of shell timeout using button below "
                                  "or from the main window menu \"Options->Change Shell Timeout\".";
            LOG_MSG(messageShort, mongo::logger::LogSeverity::Error());

            auto errorDia = new QMessageBox(QMessageBox::Icon::Critical, "Error", messageLong);
            auto but = new QPushButton("Change Shell Timeout");
            VERIFY(connect(but, SIGNAL(clicked()), this, SLOT(changeShellTimeout())));
            errorDia->addButton(but, QMessageBox::NoRole);
            errorDia->exec();
        }
    }

    void QueryWidget::activateTabContent()
    {
        AppRegistry::instance().bus()->publish(new QueryWidgetUpdatedEvent(this, _currentResult.results().size()));
        _scriptWidget->setScriptFocus();
    }

    void QueryWidget::handle(AutocompleteResponse *event)
    {
        if (event->isError()) {
            // Do not show error message (error should be already logged)
            return;
        }

        _scriptWidget->showAutocompletion(event->list, QtUtils::toQString(event->prefix) );
    }

    void QueryWidget::on_dock_undock()
    {
        if (!_dock->isFloating()) {    // If output window docked 
            // Settings to revert to docked mode
            _scriptWidget->ui_queryLinesCountChanged();
            _mainLayout->addWidget(_scriptWidget);                     
            _mainLayout->addWidget(_line);
            _mainLayout->addWidget(_outputWindow, 1);
            _dock->setFeatures(QDockWidget::DockWidgetFloatable);
            _dock->setTitleBarWidget(new QWidget);
            _viewer->applyDockUndockSettings(true);
        }
        else {              // output window undocked(floating)
            // Settings for query window in order to use maximum space
            _scriptWidget->disableFixedHeight();
            _mainLayout->addWidget(_scriptWidget, 1); 
            _mainLayout->addWidget(_line);
            _mainLayout->addWidget(_outputWindow);
            _dock->setFeatures(QDockWidget::DockWidgetClosable);
            _dock->setTitleBarWidget(nullptr);
            _viewer->applyDockUndockSettings(false);
        }
    }

    void QueryWidget::updateCurrentTab()
    {
        const QString &shellQuery = QtUtils::toQString(_shell->query());
        QString toolTipQuery = shellQuery.left(700);

        QString tabTitle, toolTipText;
        if (_shell) {
            QFileInfo fileInfo(_shell->filePath());
            if (fileInfo.isFile()) {
                    tabTitle = fileInfo.fileName();
                    toolTipText = fileInfo.filePath();
            }
        }

        if (tabTitle.isEmpty() && shellQuery.isEmpty()) {
            tabTitle = "New Shell";
        }
        else {

            if (tabTitle.isEmpty()) {
                tabTitle = shellQuery.left(41).replace(QRegExp("[\n\r\t]"), " ");
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

        _viewer->present(_shell, results);
    }
}
