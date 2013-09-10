#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

#include <Qsci/qsciscintilla.h>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoShellResult.h"

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
        QueryWidget(MongoShell *shell, WorkAreaTabWidget *tabWidget,QWidget *parent = NULL);

        bool eventFilter(QObject *o, QEvent *e);

        void toggleOrientation();
        void activateTabContent();
        void openNewTab();
        void reload();
        void duplicate();
        void enterTreeMode();
        void enterTextMode();
        void enterTableMode();
        void enterCustomMode();
        void showProgress();
        void hideProgress();
        const MongoShell *const shell() const
        {
            return _shell;
        }
        ~QueryWidget();

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
    private:
        void updateCurrentTab();
        void displayData(const std::vector<MongoShellResult> &results, bool empty);

        App *_app;
        EventBus *_bus;
        MongoShell *_shell;
        OutputWidget *_viewer;
        ScriptWidget *_scriptWidget;
        WorkAreaTabWidget *_tabWidget;
        QLabel *_outputLabel;

        MongoShellExecResult _currentResult;
        bool isTextChanged;
    };
}
