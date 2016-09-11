#pragma once

#include <QWidget>
#include <QDockWidget>
#include <QCloseEvent>

QT_BEGIN_NAMESPACE
class QLabel;
class QVBoxLayout;
class QMainWindow;
class QPushButton;
QT_END_NAMESPACE

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoShellResult.h"

namespace Robomongo
{
    class BsonWidget;
    class DocumentListLoadedEvent;
    class ScriptExecutedEvent;
    class AutocompleteResponse;
    class OutputWidget;
    class ScriptWidget;
    class MongoShell;

    class QueryWidget : public QWidget
    {
        Q_OBJECT

    public:
        class CustomDockWidget;
        typedef QWidget BaseClass;

        QueryWidget(MongoShell *shell, QWidget *parent = NULL);

        void toggleOrientation();
        void activateTabContent();
        void openNewTab();
        void reload();
        void duplicate();
        void enterTreeMode();
        void enterTextMode();
        void enterTableMode();
        void enterCustomMode();
        void setScriptFocus();
        void showAutocompletion();
        void hideAutocompletion();
        
        // Bring active tab's dock into front
        void bringDockToFront();

        // Get output window's dock status
        bool outputWindowDocked() const;

        ~QueryWidget();

    Q_SIGNALS:
        void titleChanged(const QString &text);
        void toolTipChanged(const QString &text);

    public Q_SLOTS:
        void execute();
        void stop();

        void saveToFile();
        void savebToFileAs();
        void openFile();
        void textChange();
        void showProgress();

        // Toggle output window between dock/undock
        void dockUndock();

    public Q_SLOTS:
        void handle(DocumentListLoadedEvent *event);
        void handle(ScriptExecutedEvent *event);
        void handle(AutocompleteResponse *event);

    private Q_SLOTS:
        void on_dock_undock(bool isFloating);

    private:        
        void hideProgress();
        void updateCurrentTab();
        void displayData(const std::vector<MongoShellResult> &results, bool empty);

        MongoShell *_shell;
        OutputWidget *_viewer;
        ScriptWidget *_scriptWidget;
        QLabel *_outputLabel;
        QDockWidget *_dock;
        QMainWindow *_outputWindow;
        QVBoxLayout *_mainLayout;

        MongoShellExecResult _currentResult;
        bool _isTextChanged;
    };

    /* ------- class CustomDockWidget -------- */
    /* Custom dock widget for output window */
    class QueryWidget::CustomDockWidget : public QDockWidget
    {
    public:
        CustomDockWidget(QueryWidget* parent)
            : _parent(parent)
        {}

        QueryWidget* getQueryWidget() const { return _parent; }

    protected:
        // Dock instead of close
        void closeEvent(QCloseEvent *event) override
        {
            event->ignore();
            setFloating(false);
        }
    private:
        QueryWidget* _parent;
    };
}
