#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QLabel>
#include <Qsci/qsciscintilla.h>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoShellResult.h"

namespace Robomongo
{
    class BsonWidget;
    class EventBus;
    class DocumentListLoadedEvent;
    class ScriptExecutedEvent;
    class PlainJavaScriptEditor;
    class RoboScintilla;
    class OutputWidget;
    class WorkAreaTabWidget;
    class ScriptWidget;

    class QueryWidget : public QWidget
    {
        Q_OBJECT

    public:
        QueryWidget(MongoShell *shell, WorkAreaTabWidget *tabWidget, const QString &script, bool textMode, QWidget *parent = NULL);
        ~QueryWidget() {}

        bool eventFilter(QObject * o, QEvent * e);

        void toggleOrientation();
        void activateTabContent();
        void openNewTab();
        void reload();
        void duplicate();
        void enterTreeMode();
        void enterTextMode();

        MongoShell *shell() const { return _shell; }

    public slots:
        void execute();

    public slots:
        void handle(DocumentListLoadedEvent *event);
        void handle(ScriptExecutedEvent *event);

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
        bool _textMode;

        QList<MongoShellResult> _currentResults;
    };
}
