#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QLabel>
#include <QGraphicsView>
#include <Qsci/qsciscintilla.h>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/gui/ViewMode.h"

namespace Robomongo
{
    class BsonWidget;
    class EventBus;
    class DocumentListLoadedEvent;
    class ScriptExecutedEvent;
    class AutocompleteResponse;
    class OutputWidget;
    class WorkAreaTabWidget;
    class ScriptWidget;
    class MongoShell;

    class QueryWidget : public QWidget
    {
        Q_OBJECT

    public:
        typedef QWidget BaseClass;
        QueryWidget(MongoShell *shell, WorkAreaTabWidget *tabWidget, ViewMode viewMode, QWidget *parent = NULL);

        bool eventFilter(QObject *o, QEvent *e);

        void toggleOrientation();
        void activateTabContent();
        void openNewTab();
        void reload();
        void duplicate();
        void enterTreeMode();
        void enterTextMode();
        void enterCustomMode();
        void showProgress();
        void hideProgress();
        const MongoShell *const shell() const
        {
            return _shell;
        }
    public Q_SLOTS:
        void execute();
        void stop();

        void saveToFile();
        void savebToFileAs();
        void openFile();
        void textChange();
    public Q_SLOTS:
        void handle(DocumentListLoadedEvent *event);
        void handle(ScriptExecutedEvent *event);
        void handle(AutocompleteResponse *event);
    protected:
         virtual void closeEvent(QCloseEvent *ev);
    private:
        void updateCurrentTab();
        void displayData(const QList<MongoShellResult> &results, bool empty);

        App *_app;
        EventBus *_bus;
        MongoShell *_shell;
        OutputWidget *_viewer;
        ScriptWidget *_scriptWidget;
        WorkAreaTabWidget *_tabWidget;
        QLabel *_outputLabel;
        ViewMode _viewMode;

        MongoShellExecResult _currentResult;
        bool isTextChanged;
    };
}
