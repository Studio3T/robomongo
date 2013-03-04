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
#include "robomongo/core/domain/ScriptInfo.h"
#include "robomongo/gui/ViewMode.h"

namespace Robomongo
{
    class BsonWidget;
    class EventBus;
    class DocumentListLoadedEvent;
    class ScriptExecutedEvent;
    class AutocompleteResponse;
    class PlainJavaScriptEditor;
    class RoboScintilla;
    class OutputWidget;
    class WorkAreaTabWidget;
    class ScriptWidget;

    class QueryWidget : public QWidget
    {
        Q_OBJECT

    public:
        QueryWidget(MongoShell *shell, WorkAreaTabWidget *tabWidget, const ScriptInfo &scriptInfo, ViewMode viewMode, QWidget *parent = NULL);
        ~QueryWidget() {}

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

        MongoShell *shell() const { return _shell; }

    public slots:
        void execute();
        void stop();

    public slots:
        void handle(DocumentListLoadedEvent *event);
        void handle(ScriptExecutedEvent *event);
        void handle(AutocompleteResponse *event);

    private:
        QString buildTabTitle(const QString &query);
        void updateCurrentTab();
        void displayData(const QList<MongoShellResult> &results, bool empty);

        App *_app;
        EventBus *_bus;
        MongoShell *_shell;
        KeyboardManager *_keyboard;
        OutputWidget *_viewer;
        ScriptWidget *_scriptWidget;
        WorkAreaTabWidget *_tabWidget;
        QLabel *_outputLabel;
        ViewMode _viewMode;

        QList<MongoShellResult> _currentResults;
    };
}
